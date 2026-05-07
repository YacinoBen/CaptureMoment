/**
 * @file working_image_data.cpp
 * @brief Implementation of base class for working image implementations.
 * @author CaptureMoment Team
 * @date 2026
 */

#include "image_processing/common/working_image_data.h"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace CaptureMoment::Core::ImageProcessing {


WorkingImageData::WorkingImageData(std::unique_ptr<Common::ImageRegion> source)
{
    if (!source || !source->isValid()) {
        spdlog::warn("[WorkingImageData]: Invalid source image provided");
        return;  // m_valid stays false, derived can check
    }

    auto result { initializeData(std::move(*source)) };
    if (!result) {
        spdlog::error("[WorkingImageData]: Init failed: {}",
                         ErrorHandling::to_string(result.error()));
    } else {
        spdlog::debug("[WorkingImageData]: Initialized {}x{} ({} ch)",
                         m_width, m_height, m_channels);
    }
}

std::expected<void, ErrorHandling::CoreError> WorkingImageData::initializeData(Common::ImageRegion&& source_image)
{
    if (!source_image.isValid()) {
        spdlog::warn("[WorkingImageData::initializeData]: Input ImageRegion is invalid");
        return std::unexpected(ErrorHandling::CoreError::InvalidImageRegion);
    }

    try {
        // Cache dimensions and validity
        m_data = std::move(source_image.m_data);

        m_original_data = m_data;

        m_width = source_image.width();
        m_height = source_image.height();
        m_channels = source_image.channels();
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
    if (!m_valid || m_original_data.empty() || m_data.empty()) {
        return;
    }

    if (m_original_data.size() != m_data.size()) {
        spdlog::error("[WorkingImageData::restoreOriginalData]: Size mismatch! Original: {}, Working: {}",
                      m_original_data.size(), m_data.size());
        return;
    }

    std::ranges::copy(m_original_data.begin(), m_original_data.end(), m_data.begin());
}

} // namespace CaptureMoment::Core::ImageProcessing
