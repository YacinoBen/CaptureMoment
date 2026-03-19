/**
 * @file working_image_cpu_halide.cpp
 * @brief Implementation of WorkingImageCPU_Halide
 * @author CaptureMoment Team
 * @date 2025
 */

#include "image_processing/cpu/working_image_cpu_halide.h"

#include <spdlog/spdlog.h>
#include <utility>
#include <cstring>

namespace CaptureMoment::Core::ImageProcessing {

WorkingImageCPU_Halide::WorkingImageCPU_Halide(std::unique_ptr<Common::ImageRegion> initial_image)
{
    if (initial_image && initial_image->isValid()) {
        // Use move semantics to initialize efficiently
        auto result = updateFromCPU(std::move(*initial_image));

        if (!result) {
            spdlog::error("[WorkingImageCPU_Halide]: Constructor failed to initialize. Reason: {}", ErrorHandling::to_string(result.error()));
        } else {
            spdlog::debug("[WorkingImageCPU_Halide]: Constructed and initialized with valid initial image ({}x{}, {} ch)",
                          m_halide_buffer.width(), m_halide_buffer.height(), m_halide_buffer.channels());
        }
    } else {
        spdlog::debug("[WorkingImageCPU_Halide]: Constructed with no initial image or invalid image data");
    }
}

std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError>
WorkingImageCPU_Halide::convertHalideToImageRegion()
{
    try {
        auto cpu_image_copy = std::make_unique<Common::ImageRegion>();

        // Set dimensions and format from the Halide buffer
        cpu_image_copy->m_width = static_cast<int>(m_halide_buffer.width());
        cpu_image_copy->m_height = static_cast<int>(m_halide_buffer.height());
        cpu_image_copy->m_channels = static_cast<int>(m_halide_buffer.channels());
        cpu_image_copy->m_format = Common::PixelFormat::RGBA_F32; // Assuming F32 for now

        std::span<const float> data_span = getDataSpan();
        if (data_span.empty()) {
            spdlog::warn("[WorkingImageCPU_Halide::convertHalideToImageRegion]: Data span is empty, cannot export");
            return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
        }

        // Deep copy from unique_ptr back to ImageRegion's vector
        cpu_image_copy->m_data.assign(data_span.begin(), data_span.end());

        if (!cpu_image_copy->isValid()) {
            spdlog::warn("[WorkingImageCPU_Halide::convertHalideToImageRegion]: Created ImageRegion is invalid after copying data");
            return std::unexpected(ErrorHandling::CoreError::InvalidImageRegion);
        }

        spdlog::debug("[WorkingImageCPU_Halide::convertHalideToImageRegion]: Exported ImageRegion ({}x{}, {} ch)",
                      cpu_image_copy->m_width, cpu_image_copy->m_height, cpu_image_copy->m_channels);

        return cpu_image_copy;

    } catch (const std::bad_alloc& e) {
        spdlog::critical("[WorkingImageCPU_Halide::convertHalideToImageRegion]: Failed to allocate memory: {}", e.what());
        return std::unexpected(ErrorHandling::CoreError::AllocationFailed);
        } catch (const std::exception& e) {
        spdlog::critical("[WorkingImageCPU_Halide::convertHalideToImageRegion]: Exception: {}", e.what());
            return std::unexpected(ErrorHandling::CoreError::Unexpected);
    }
}


std::expected<void,  ErrorHandling::CoreError>
WorkingImageCPU_Halide::updateFromCPU(const Common::ImageRegion& cpu_image)
{
    if (!cpu_image.isValid()) {
        spdlog::warn("[WorkingImageCPU_Halide::updateFromCPU]: Input ImageRegion is invalid");
        return std::unexpected(ErrorHandling::CoreError::InvalidImageRegion);
    }

    auto result = initializeData(cpu_image);
    if (!result) {
        spdlog::error("[WorkingImageCPU_Halide::updateFromCPU]: Failed to initialize and copy from CPU image. Reason: {}",
                    ErrorHandling::to_string(result.error()));
        return std::unexpected(result.error());
    }

    initializeHalide(getDataSpan(),
                     static_cast<int>(m_width),
                     static_cast<int>(m_height),
                     static_cast<int>(m_channels));


    spdlog::debug("[WorkingImageCPU_Halide::updateFromCPU]: Updated from CPU image ({}x{}, {} ch)",
                      m_width, m_height, m_channels);

    return {}; // Success
}

std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError>
WorkingImageCPU_Halide::exportToCPUCopy()
{
    if (!isValid()) {
        spdlog::warn("[WorkingImageCPU_Halide::exportToCPUCopy]: Current Halide buffer is invalid, cannot export");
        return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
    }

    return convertHalideToImageRegion();
}

std::pair<Common::ImageDim, Common::ImageDim> WorkingImageCPU_Halide::getSize() const
{
    if (!isValid()) {
        return {0, 0};
    }
    return getSizeByHalide();
}

Common::ImageChan WorkingImageCPU_Halide::getChannels() const
{
    if (!isValid()) {
        return 0;
    }
    return getChannelsByHalide();
}

Common::ImageSize WorkingImageCPU_Halide::getPixelCount() const
{
    if (!isValid()) {
        return 0;
    }
    return getPixelCountByHalide();
}

Common::ImageSize WorkingImageCPU_Halide::getDataSize() const
{
    if (!isValid()) {
        return 0;
    }
    return getDataSizeByHalide();
}

} // namespace CaptureMoment::Core::ImageProcessing
