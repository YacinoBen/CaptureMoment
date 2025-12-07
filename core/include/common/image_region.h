/**
 * @file image_region.h
 * @brief Structure represent ing a pixel buffer for an image region
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once
#include "pixel_format.h"
#include <vector>
#include <cstdint>

namespace CaptureMoment {

/**
 * @struct ImageRegion
 * @brief Buffer of pixels representing an image region
 * 
 * This structure contains the raw data of a rectangular region
 * of an image. It is used as a working buffer between different
 * stages of the pipeline (loading → processing → display).
 * 
 * @par Architecture
 * Pixels are stored in a **row-major** layout:
 * @code
 * For a 3x2 image in RGBA:
 * data = [R₀ G₀ B₀ A₀ | R₁ G₁ B₁ A₁ | R₂ G₂ B₂ A₂]  ← Row 0
 *        [R₃ G₃ B₃ A₃ | R₄ G₄ B₄ A₄ | R₅ G₅ B₅ A₅]  ← Row 1
 * @endcode
 * 
 * @par Life Cycle
 * 1. **Creation** : SourceManager::getTile() allocates and fills
 * 2. **Processing** : IOperation::execute() modifies in-place
 * 3. **Display** : RenderManager::toQImage() converts to Qt
 * 
 * @warning Data is in **float32** during processing (HDR).
 *          Remember to clamp [0.0, 1.0] before uint8 conversion    .
 * 
 * @see SourceManager::getTile()
 * @see IOperation::execute()
 * @see RenderManager::toQImage()
 */
struct ImageRegion {
    /**
     * @brief X position of the top-left corner (in pixels)
     * @note Coordinates relative to the full source image
     */
    int m_x{0};
    
    /**
     * @brief Y position of the top-left corner (in pixels)
     */
    int m_y{0};
    
    /**
     * @brief Width of the region (in pixels)
     */
    int m_width{0};
    
    /**
     * @brief Height of the region (in pixels)
     */
    int m_height{0};
    
    /**
     * @brief Number of channels per pixel
     * @details
     * - 3 for RGB
     * - 4 for RGBA
     * @note Must match the format (e.g., RGBA_F32 → channels=4)
     */
    int m_channels{4};
    
    /**
     * @brief Storage format of the pixels
     * @see PixelFormat
     */
    PixelFormat m_format{PixelFormat::RGBA_F32};
    
    /**
     * @brief Pixel data (row-major layout)
     * 
     * @par Vector size
     * `m_data.size() = m_width × m_height × m_channels`
     * 
     * @par Manual access example
     * @code
     * ImageRegion region;
     * size_t index = (20 * region.m_width + 10) * region.m_channels + 0;
     * float red = region.m_data[index];
     * @endcode
     * 
     * @note Prefer the operator()(y, x, c) for safer access
     */
    std::vector<float> m_data;
    
    /**
     * @brief Checks the validity of the region
     * @return true if the region is valid and usable
     * @retval false if width ≤ 0, height ≤ 0 or data is empty
     * 
     * @code
     * auto region = sourceManager->getTile(0, 0, 100, 100);
     * if (region && region->isValid()) {
     * }
     * @endcode
     */
    [[nodiscard]] constexpr bool isValid() const noexcept {
    // 1. Check for positive dimensions (including channels)
    if (m_width <= 0 || m_height <= 0 || m_channels <= 0) {
        return false;
    }

    // 2. Check if the vector size matches the calculated size
    // This is crucial to detect corrupted or incorrectly resized regions.
    const size_t expected_size = static_cast<size_t>(m_width) * m_height * m_channels;
    return m_data.size() == expected_size;
    }
    
    /**
     * @brief Calculates the size in bytes of the data buffer
     * @return Total size in bytes
     * 
     * @code
     * ImageRegion region;
     * region.width = 1920;
     * region.height = 1080;
     * region.channels = 4;
     * size_t bytes = region.sizeInBytes();
     * @endcode
     */
    [[nodiscard]] constexpr size_t sizeInBytes() const noexcept {
        return m_data.size() * sizeof(float);
    }
    
    /**
     * @brief Safe access to a pixel (non-const)
     * 
     * @param y Y coordinate (row)
     * @param x X coordinate (column)
     * @param c Channel (0=R, 1=G, 2=B, 3=A)
     * @return Reference to the pixel value at (y, x, c)
     * 
     * @warning No bounds checking. The caller must ensure:
     *          - 0 ≤ y < height
     *          - 0 ≤ x < width
     *          - 0 ≤ c < channels
     * 
     * @code
     * ImageRegion region;
     * region(20, 10, 0) = 1.0f;  // Red = max
     * @endcode
     */
    [[nodiscard]] float& operator()(int y, int x, int c) noexcept {
        return m_data[static_cast<size_t>((y * m_width + x) * m_channels + c)];
    }
    
    /**
     * @brief Safe access to a pixel (const)
     * @see operator()(int, int, int)
     */
    [[nodiscard]] const float& operator()(int y, int x, int c) const noexcept {
        return m_data[static_cast<size_t>((y * m_width + x) * m_channels + c)];
    }
};

} // namespace CaptureMoment