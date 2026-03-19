/**
 * @file working_image_gpu_halide.cpp
 * @brief Implementation of WorkingImageGPU_Halide.
 * @details Relies on AppConfig to provide the active Halide Target.
 *          Responsibility is strictly execution, not device selection.
 * @author CaptureMoment Team
 * @date 2026
 */

#include "image_processing/gpu/working_image_gpu_halide.h"
#include "config/app_config.h"
#include <spdlog/spdlog.h>
#include <utility>
#include <cstring>

#include "HalideRuntime.h"

namespace CaptureMoment::Core::ImageProcessing {


WorkingImageGPU_Halide::WorkingImageGPU_Halide(std::unique_ptr<Common::ImageRegion> initial_image)
{
    // We assume AppConfig::getHalideTarget() has been correctly initialized
    // by IBackendDecider at application startup.
    if (initial_image && initial_image->isValid())
    {
        auto result = updateFromCPU(*initial_image);
        if (!result)
        {
            spdlog::error("[WorkingImageGPU_Halide::WorkingImageGPU_Halide]: Init failed: {}",
                          ErrorHandling::to_string(result.error()));
        }
    }
    else
    {
        spdlog::debug("[WorkingImageGPU_Halide::WorkingImageGPU_Halide]: Constructed with no initial image or invalid image data");
    }
}


std::expected<void, ErrorHandling::CoreError>
WorkingImageGPU_Halide::updateFromCPU(const Common::ImageRegion& cpu_image)
{
    auto result = initializeData(cpu_image);
    if (!result) {
        return result;
    }

    initializeHalide(getDataSpan(),
                     static_cast<int>(m_width),
                     static_cast<int>(m_height),
                     static_cast<int>(m_channels));

    // Transfer to GPU
    Halide::Target target = Config::AppConfig::getHalideTarget();
    m_halide_buffer.set_host_dirty();
    int gpu_result = m_halide_buffer.copy_to_device(target);

    if (gpu_result != 0) {
        spdlog::critical("[WorkingImageGPU_Halide::updateFromCPU]: copy_to_device failed: {}", gpu_result);
        return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
    }

    spdlog::debug("[WorkingImageGPU_Halide::updateFromCPU]: Updated ({}x{}, {} ch)",
                  m_width, m_height, m_channels);

    return {};
}


std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError>
WorkingImageGPU_Halide::exportToCPUCopy()
{
    if (!isValid())
    {
        return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
    }

    try
    {
        // Sync GPU → CPU
        int result = m_halide_buffer.copy_to_host();
        if (result != 0) {
            spdlog::critical("[WorkingImageGPU_Halide::exportToCPUCopy]: copy_to_host failed: {}", result);
            return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
        }

        // Copy to ImageRegion
        std::vector<float> copied_data(m_data_size);
        std::memcpy(copied_data.data(), m_data.get(), m_data_size * sizeof(float));

        auto region = std::make_unique<Common::ImageRegion>(
            std::move(copied_data),
            static_cast<int>(m_width),
            static_cast<int>(m_height),
            static_cast<int>(m_channels)
        );
        region->m_format = Common::PixelFormat::RGBA_F32;

        return region;

    }
    catch (const std::bad_alloc& e)
    {
        spdlog::critical("[WorkingImageGPU_Halide::exportToCPUCopy]: Failed to allocate memory: {}", e.what());
        return std::unexpected(ErrorHandling::CoreError::AllocationFailed);
    }
    catch (const std::exception& e)
    {
        spdlog::critical("[WorkingImageGPU_Halide::exportToCPUCopy]: Exception: {}", e.what());
        return std::unexpected(ErrorHandling::CoreError::InvalidImageRegion);
    }
}

std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError>
WorkingImageGPU_Halide::downsample(Common::ImageDim target_width, Common::ImageDim target_height)
{
    if (!m_valid) {
        return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
    }

    try {
        Halide::Target target = Config::AppConfig::getHalideTarget();

        // GPU downsample
        Halide::Func downsample("downsample_gpu");
        Halide::Var x, y, c;

        float scale_x = static_cast<float>(m_width) / target_width;
        float scale_y = static_cast<float>(m_height) / target_height;

        // Use bilinear interpolation for better quality
        Halide::Expr src_x = x * scale_x;
        Halide::Expr src_y = y * scale_y;

        downsample(x, y, c) = m_halide_buffer(
            Halide::cast<int>(src_x),
            Halide::cast<int>(src_y),
            c
            );

        Halide::Var xi, yi;
        downsample.gpu_tile(x, y, xi, yi, 16, 16);

        // Realize to a temporary buffer
        Halide::Buffer<float> result_buf = downsample.realize(
            {static_cast<int>(target_width),
             static_cast<int>(target_height),
             static_cast<int>(m_channels)},
            target
            );

        // Copy to host
        result_buf.copy_to_host();

        // Check strides
        spdlog::debug("[WorkingImageGPU_Halide::downsample]: strides = {}, {}, {}",
                      result_buf.dim(0).stride(), result_buf.dim(1).stride(), result_buf.dim(2).stride());

        // Copy data to contiguous vector
        size_t result_size = target_width * target_height * m_channels;
        std::vector<float> result_data(result_size);

        // Manual copy respecting strides
        for (int c = 0; c < static_cast<int>(m_channels); ++c) {
            for (int y = 0; y < static_cast<int>(target_height); ++y) {
                for (int x = 0; x < static_cast<int>(target_width); ++x) {
                    size_t dst_idx = (y * target_width + x) * m_channels + c;
                    result_data[dst_idx] = result_buf(x, y, c);
                }
            }
        }

        auto region = std::make_unique<Common::ImageRegion>(
            std::move(result_data),
            static_cast<int>(target_width),
            static_cast<int>(target_height),
            static_cast<int>(m_channels)
            );
        region->m_format = Common::PixelFormat::RGBA_F32;

        spdlog::debug("[WorkingImageGPU_Halide::downsample]: Downsampled {}x{} → {}x{}",
                      m_width, m_height, target_width, target_height);

        return region;
    }
    catch (const std::exception& e) {
        spdlog::critical("[WorkingImageGPU_Halide::downsample]: Downsample failed: {}", e.what());
        return std::unexpected(ErrorHandling::CoreError::AllocationFailed);
    }
}

std::pair<Common::ImageDim, Common::ImageDim> WorkingImageGPU_Halide::getSize() const
{
    if (!isValid()) {
        return {0, 0};
    }
    return getSizeByHalide();
}

Common::ImageChan WorkingImageGPU_Halide::getChannels() const
{
    if (!isValid()) {
        return 0;
    }
    return getChannelsByHalide();
}

Common::ImageSize WorkingImageGPU_Halide::getPixelCount() const
{
    if (!isValid()) {
        return 0;
    }
    return getPixelCountByHalide();
}

Common::ImageSize WorkingImageGPU_Halide::getDataSize() const
{
    if (!isValid()) {
        return 0;
    }
    return getDataSizeByHalide();
}

} // namespace CaptureMoment::Core::ImageProcessing
