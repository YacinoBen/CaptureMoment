/**
 * @file pixel_format.h
 * @brief Definitions of pixel storage formats in memory
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once
#include <cstdint>

namespace CaptureMoment {

/**
 * @enum PixelFormat
 * @brief Pixel storage format in memory
 * 
 * Defines the data structure for each pixel:
 * - Number of channels (RGB vs RGBA)
 * - Data type (float32 for HDR, uint8 for display)
 * 
 * @note The RGBA_F32 format is recommended for the processing pipeline
 *       as it preserves the dynamic range (HDR).
 */
enum class PixelFormat : uint8_t {
    /**
     * @brief 4 channels (Red, Green, Blue, Alpha) in float32
     * 
     * - Size per pixel: 16 bytes (4 × 4 bytes)
     * - Value range: [-∞, +∞] (but usually [0.0, 1.0])
     * - Usage: Internal processing pipeline, HDR
     * 
     * @code
     * ImageRegion region;
     * region.format = PixelFormat::RGBA_F32;
     * region.channels = 4;
     * @endcode
     */
    RGBA_F32,
    
    /**
     * @brief 3 channels (Red, Green, Blue) in float32
     * 
     * - Size per pixel: 12 bytes (3 × 4 bytes)
     * - Usage: Files without alpha channel
     */
    RGB_F32,
    
    /**
     * @brief 4 channels in uint8 (0-255)
     * 
     * - Size per pixel: 4 bytes
     * - Usage: Final display (QImage, GPU texture)
     */
    RGBA_U8,
    
    /**
     * @brief 3 channels in uint8
     * 
     * - Size per pixel: 3 bytes
     * - Usage: Export JPEG
     */
    RGB_U8
};

} // namespace CaptureMoment
