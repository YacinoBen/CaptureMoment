/**
 * @file working_image_gpu_halide.cpp
 * @brief Implementation of WorkingImageGPU_Halide.
 * @details Implements real GPU transfers via Halide runtime.
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

bool WorkingImageGPU_Halide::bindView(const ImageView& view)
{

    if (!IWorkingImageHardware::bindView(view)) {
        return false;
    }

    // 2. Initialise le buffer Halide sur le device
    initializeHalide(m_view_data_image.working_data,
                     m_view_data_image.width,
                     m_view_data_image.height,
                     m_view_data_image.channels);

    if (!isHalideBufferValid()) {
        spdlog::error("[WorkingImageGPU_Halide::bindView]: Failed to initialize Halide buffer on device.");
        return false;
    }

    spdlog::debug("[WorkingImageGPU_Halide::bindView]: Bound and initialized GPU Halide buffer ({}x{}).",
                  m_view_data_image.width, m_view_data_image.height);
    return true;
}

std::expected<void, ErrorHandling::CoreError>
WorkingImageGPU_Halide::updateFromCPU()
{
    if (!isHalideBufferValid()) {
        return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
    }

    try {
        // 1. marker the buffer as dirty to ensure Halide knows it needs to upload the new data
        m_halide_buffer.set_host_dirty();

        // 2. transfert to the GPU
        Halide::Target target { Config::AppConfig::getHalideTarget() };
        int gpu_result { m_halide_buffer.copy_to_device(target) };

        if (gpu_result != 0) {
            spdlog::critical("[WorkingImageGPU_Halide::updateFromCPU]: copy_to_device failed with code: {}", gpu_result);
            return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
        }

        spdlog::debug("[WorkingImageGPU_Halide::updateFromCPU]: Uploaded to GPU ({}x{}, {} ch)",
                      m_view_data_image.width, m_view_data_image.height, m_view_data_image.channels);
        return {};
    } catch (const std::exception& e) {
        spdlog::critical("[WorkingImageGPU_Halide::updateFromCPU]: Exception: {}", e.what());
        return std::unexpected(ErrorHandling::CoreError::Unexpected);
    }
}

std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError>
WorkingImageGPU_Halide::exportToCPUCopy()
{
    if (!isValid()) {
        return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
    }

    try {
        // 1. force the buffer to copy back to host (synchronous)
        int result = m_halide_buffer.copy_to_host();
        if (result != 0) {
            spdlog::critical("[WorkingImageGPU_Halide::exportToCPUCopy]: copy_to_host failed with code: {}", result);
            return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
        }

        // 2. wait for the copy to complete
        m_halide_buffer.device_sync();

        // 3. Create a new ImageRegion and copy data from the view's span (which points to the CPU RAM)
        auto cpu_image_copy { std::make_unique<Common::ImageRegion>() };
        cpu_image_copy->m_data.assign(m_view_data_image.working_data.begin(), m_view_data_image.working_data.end());

        auto [w, h] = getSizeByHalide();
        cpu_image_copy->m_width = static_cast<int>(w);
        cpu_image_copy->m_height = static_cast<int>(h);
        cpu_image_copy->m_channels = static_cast<int>(getChannelsByHalide());
        cpu_image_copy->m_format = Common::PixelFormat::RGBA_F32;

        if (!cpu_image_copy->isValid()) {
            return std::unexpected(ErrorHandling::CoreError::InvalidImageRegion);
        }

        return cpu_image_copy;

    } catch (const std::bad_alloc& e) {
        spdlog::critical("[WorkingImageGPU_Halide::exportToCPUCopy]: Allocation failed: {}", e.what());
        return std::unexpected(ErrorHandling::CoreError::AllocationFailed);
    } catch (const std::exception& e) {
        spdlog::critical("[WorkingImageGPU_Halide::exportToCPUCopy]: Exception: {}", e.what());
        return std::unexpected(ErrorHandling::CoreError::Unexpected);
    }
}

std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError>
WorkingImageGPU_Halide::downsample(Common::ImageDim target_width, Common::ImageDim target_height)
{
    if (!isValid() || m_view_data_image.working_data.empty()) {
        return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
    }

    try {
        Halide::Target target { Config::AppConfig::getHalideTarget() };

        Halide::Func downsample("downsample_gpu");
        Halide::Var x, y, c;

        float scale_x { static_cast<float>(m_view_data_image.width) / target_width };
        float scale_y { static_cast<float>(m_view_data_image.height) / target_height };

        Halide::Expr src_x = x * scale_x;
        Halide::Expr src_y = y * scale_y;

        downsample(x, y, c) = m_halide_buffer(
            Halide::cast<int>(src_x),
            Halide::cast<int>(src_y),
            c
            );

        Halide::Var xi, yi;
        downsample.gpu_tile(x, y, xi, yi, 16, 16);

        // Realize directly into a new buffer
        Halide::Buffer<float> result_buf = downsample.realize(
            {static_cast<int>(target_width),
             static_cast<int>(target_height),
             static_cast<int>(m_view_data_image.channels)},
            target
            );

        // Synchronize device to host memory
        result_buf.copy_to_host();
        result_buf.device_sync();

        // High-performance copy: directly construct vector from Halide's raw pointer.
        // Halide handles internal strides automatically during copy_to_host.
        size_t total_elements { static_cast<size_t>(target_width) * 
                                static_cast<size_t>(target_height) * 
                                static_cast<size_t>(m_view_data_image.channels) };

        std::vector<float> result_data(
            result_buf.data(),
            result_buf.data() + total_elements
        );

        auto region { std::make_unique<Common::ImageRegion>(
            std::move(result_data),
            static_cast<int>(target_width),
            static_cast<int>(target_height),
            static_cast<int>(m_view_data_image.channels)
        ) };
        region->m_format = Common::PixelFormat::RGBA_F32;

        spdlog::debug("[WorkingImageGPU_Halide::downsample]: Downsampled {}x{} → {}x{}",
                      m_view_data_image.width, m_view_data_image.height, target_width, target_height);

        return region;
    }
    catch (const std::exception& e) {
        spdlog::critical("[WorkingImageGPU_Halide::downsample]: Downsample failed: {}", e.what());
        return std::unexpected(ErrorHandling::CoreError::AllocationFailed);
    }
}

bool WorkingImageGPU_Halide::isValid() const {
    return WorkingImageGPU::isValid() && isHalideBufferValid();
}

} // namespace CaptureMoment::Core::ImageProcessing
