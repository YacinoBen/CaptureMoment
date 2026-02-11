
/**
 * @file image_conversion.h
 * @brief Declarations of image conversion utilities
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "common/image_region.h"
#include <vector>
#include <memory>

namespace CaptureMoment::Core {

namespace Utils {

/**
 * @brief Converts an RGBA_F32 ImageRegion to a new RGBA_U8 ImageRegion.
 * @param input The source RGBA_F32 region.
 * @return A new ImageRegion with RGBA_U8 data, or nullptr if input is invalid.
 */
[[nodiscard]] std::unique_ptr<Common::ImageRegion> convert_RGBA_F32_to_RGBA_U8(const Common::ImageRegion& input);

} // namespace Utils
} // namespace CaptureMoment::Core