/**
 * @file image_region.cpp
 * @brief Implementation of ImageRegion methods
 * @author CaptureMoment Team
 * @date 2025
 */

#include "image_region.h"
namespace CaptureMoment {

    // For future-proofing, define the isValid() method outside the struct
    [[nodiscard]]
    constexpr bool ImageRegion::isValid() const noexcept {
            // 1. Check for positive dimensions (including channels)
    if (m_width <= 0 || m_height <= 0 || m_channels <= 0) {
        return false;
    }

    // 2. Check if the vector size matches the calculated size
    // This is crucial to detect corrupted or incorrectly resized regions.
    const size_t expected_size = static_cast<size_t>(m_width) * m_height * m_channels;
    return m_data.size() == expected_size;
    }

} // namespace CaptureMoment