/**
 * @file working_image_cpu_default.cpp
 * @brief Implementation of WorkingImageCPU_Default.
 * @author CaptureMoment Team
 * @date 2026
 */

#include "image_processing/cpu/working_image_cpu_default.h"
#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::ImageProcessing {

std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError>
WorkingImageCPU_Default::getFullResImage()
{
    if (!isValid()) {
        spdlog::warn("[WorkingImageCPU_Default::getFullResImage]: Image view is invalid.");
        return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
    }

    try {
        auto cpu_image_copy { std::make_unique<Common::ImageRegion>(
            m_view_data_image.working_data,
            static_cast<int>(m_view_data_image.width),
            static_cast<int>(m_view_data_image.height),
            static_cast<int>(m_view_data_image.channels)
        ) };

        if (!cpu_image_copy->isValid()) {
            return std::unexpected(ErrorHandling::CoreError::InvalidImageRegion);
        }

        spdlog::debug("[WorkingImageCPU_Default::getFullResImage]: Exported {}x{}.",
                      m_view_data_image.width, m_view_data_image.height);

        return cpu_image_copy;

    } catch (const std::bad_alloc& e) {
        spdlog::critical("[WorkingImageCPU_Default::getFullResImage]: Allocation failed: {}", e.what());
        return std::unexpected(ErrorHandling::CoreError::AllocationFailed);
    }
}

} // namespace CaptureMoment::Core::ImageProcessing
