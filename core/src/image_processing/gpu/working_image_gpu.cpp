/**
 * @file working_image_gpu.cpp
 * @brief Implementation of WorkingImageGPU.
 * @author CaptureMoment Team
 * @date 2026
 */

#include "image_processing/gpu/working_image_gpu.h"

#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::ImageProcessing {

bool WorkingImageGPU::isValid() const
{
    return !m_view_data_image.working_data.empty() && m_view_data_image.width > 0;
}

std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError>
WorkingImageGPU::getFullResImage()
{
    if (!isValid()) {
        spdlog::warn("[WorkingImageGPU::getFullResImage]: Invalid working image. Cannot export to CPU.");
        return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
    }

    // Download the GPU buffer to Host RAM (m_view_data_image.working_data)
    if (!downloadDeviceToHost()) {
        spdlog::warn("[WorkingImageGPU::getFullResImage]: Failed to download GPU buffer to Host memory.");
        return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
    }

    try {
        // Create a deep copy of the working data into a new ImageRegion
        auto cpu_image_copy {std::make_unique<Common::ImageRegion>()};

        cpu_image_copy->m_data.assign(m_view_data_image.working_data.begin(), m_view_data_image.working_data.end());

        cpu_image_copy->m_width = static_cast<int>(m_view_data_image.width);
        cpu_image_copy->m_height = static_cast<int>(m_view_data_image.height);
        cpu_image_copy->m_channels = static_cast<int>(m_view_data_image.channels);
        cpu_image_copy->m_format = Common::PixelFormat::RGBA_F32;

        if (!cpu_image_copy->isValid()) {
            return std::unexpected(ErrorHandling::CoreError::InvalidImageRegion);
        }

        return cpu_image_copy;

    } catch (const std::bad_alloc&) {
        return std::unexpected(ErrorHandling::CoreError::AllocationFailed);
    } catch (const std::exception& e) {
        return std::unexpected(ErrorHandling::CoreError::Unexpected);
    }
}

} // namespace CaptureMoment::Core::ImageProcessing
