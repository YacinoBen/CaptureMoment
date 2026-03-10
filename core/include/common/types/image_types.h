/**
 * @file image_types.h
 * @brief Common type definitions for all image types in CaptureMoment.
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include <cstddef>
#include <cstdint>

namespace CaptureMoment::Core {

namespace Common {
/**
 * @brief Type for image dimensions (width, height).
 * @details Native word size, supports large images (64MP+).
 */
using ImageDim = std::size_t;

/**
 * @brief Type for channel count.
 * @details Max 4 channels (RGBA), uint8_t sufficient.
 */
using ImageChan = std::uint8_t;

/**
 * @brief Type for pixel coordinates (x, y).
 * @details Signed for negative offsets in region calculations.
 */
using ImageCoord = std::int32_t;

/**
 * @brief Type for data size (element count).
 * @details Large images require size_t.
 */
using ImageSize = std::size_t;

} // namespace Common
} // namespace CaptureMoment::Core
