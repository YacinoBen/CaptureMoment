/**
 * @file working_image_data.cpp
 * @brief Implementation of base class for working image implementations.
 * @author CaptureMoment Team
 * @date 2026
 */

#include "image_processing/common/working_image_data.h"
#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::ImageProcessing {

std::expected<void, ErrorHandling::CoreError> WorkingImageData::initializeData(const Common::ImageRegion& cpu_image)
{
    if (!cpu_image.isValid()) {
        spdlog::warn("[WorkingImageData::initializeData]: Input ImageRegion is invalid");
        return std::unexpected(ErrorHandling::CoreError::InvalidImageRegion);
    }

    try {
        size_t required_size = static_cast<size_t>(cpu_image.m_width) *
                               cpu_image.m_height *
                               cpu_image.m_channels;

        // Check allocation size and use No-Init allocation
        if (!m_data || m_data_size != required_size) {
            m_data = std::make_unique_for_overwrite<float[]>(required_size);
            m_data_size = required_size;
        }

        // Fast memory copy
        std::memcpy(m_data.get(), cpu_image.m_data.data(), required_size * sizeof(float));

        // Cache dimensions and validity
        m_width = cpu_image.m_width;
        m_height = cpu_image.m_height;
        m_channels = cpu_image.m_channels;
        m_valid = true;

        spdlog::debug("[WorkingImageData::initializeData]: Copied {} elements ({}x{}, {} ch)",
                      required_size, m_width, m_height, m_channels);

        return {};

    } catch (const std::bad_alloc& e) {
        spdlog::critical("[WorkingImageData::initializeData]: Allocation failed: {}", e.what());
        return std::unexpected(ErrorHandling::CoreError::AllocationFailed);
    }
}

} // namespace CaptureMoment::Core::ImageProcessing
