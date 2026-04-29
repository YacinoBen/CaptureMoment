/**
 * @file working_image_data.cpp
 * @brief Implementation of base class for working image implementations.
 * @author CaptureMoment Team
 * @date 2026
 */

#include "image_processing/common/working_image_data.h"
#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::ImageProcessing {

std::expected<void, ErrorHandling::CoreError> WorkingImageData::initializeData(Common::ImageRegion&& cpu_image)
{
    if (!cpu_image.isValid()) {
        spdlog::warn("[WorkingImageData::initializeData]: Input ImageRegion is invalid");
        return std::unexpected(ErrorHandling::CoreError::InvalidImageRegion);
    }

    try {
        // Cache dimensions and validity
        m_data = std::move(cpu_image.m_data);

        if (m_original_data.empty()) {
            m_original_data = m_data;
        }

        m_width = cpu_image.width();
        m_height = cpu_image.height();
        m_channels = cpu_image.channels();
        m_valid = true;

        spdlog::debug("[WorkingImageData::initializeData]: Copied elements ({}x{}, {} ch)",
                      m_width, m_height, m_channels);

        return {};

    } catch (const std::bad_alloc& e) {
        spdlog::critical("[WorkingImageData::initializeData]: Allocation failed: {}", e.what());
        return std::unexpected(ErrorHandling::CoreError::AllocationFailed);
    }
}

void WorkingImageData::restoreOriginalData()
{
    if (m_original_data.empty() || m_data.empty()) {
        return;
    }

    if (m_original_data.size() != m_data.size()) {
        spdlog::error("[WorkingImageData::restoreOriginalData]: Size mismatch! Original: {}, Working: {}",
                      m_original_data.size(), m_data.size());
        return;
    }

    std::memcpy(m_data.data(), m_original_data.data(), m_data.size() * sizeof(float));
}

} // namespace CaptureMoment::Core::ImageProcessing
