/**
 * @file image_region.h
 * @brief Defines the ImageRegion structure for representing pixel buffers in the CaptureMoment Core library.
 *
 * This header provides the @ref CaptureMoment::Core::Common::ImageRegion struct,
 * which encapsulates the raw pixel data for a rectangular region of an image.
 * It is the primary data container used throughout the image processing pipeline,
 * facilitating data transfer between stages like loading (@ref SourceManager),
 * processing (@ref IOperation), and display preparation.
 *
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include "pixel_format.h" // Defines the PixelFormat enum
#include <vector>         // Required for std::vector<float> storage
#include <cstddef>        // Required for size_t
#include <concepts>

namespace CaptureMoment::Core {

namespace Common{
/**
 * @struct ImageRegion
 * @brief Represents a rectangular region of an image and its pixel data.
 *
 * This structure holds the raw pixel values for a specified rectangular area
 * of an image. It defines the spatial location (`m_x`, `m_y`), dimensions
 * (`m_width`, `m_height`), color format (`m_channels`, `m_format`), and the
 * actual pixel data (`m_data`). It serves as the fundamental unit of data
 * processed by the pipeline stages.
 *
 * @par Memory Layout
 * The pixel data in `m_data` is stored in a strict **row-major order**.
 * For an image region with C channels, W width, and H height, the pixel
 * at coordinates (x, y, c) (where c is the channel index) is located at:
 * `m_data[(y * W + x) * C + c]`.
 *
 * Example layout for a 3x2 image region in RGBA format:
 * @code
 * // Memory layout (row-major):
 * // [R₀₀ G₀₀ B₀₀ A₀₀ R₀₁ G₀₁ B₀₁ A₀₁ R₀₂ G₀₂ B₀₂ A₀₂] // Row 0
 * // [R₁₀ G₁₀ B₁₀ A₁₀ R₁₁ G₁₁ B₁₁ A₁₁ R₁₂ G₁₂ B₁₂ A₁₂] // Row 1
 * // Data vector: m_data = {R₀₀, G₀₀, B₀₀, A₀₀, R₀₁, G₀₁, B₀₁, A₀₁, ...};
 * @endcode
 *
 * @par Usage Lifecycle
 * 1. **Creation/Loading:** Typically initialized by @ref SourceManager::getTile,
 *    which allocates `m_data` and fills it with pixel values from an image file.
 * 2. **Processing:** Modified in-place by @ref IOperation implementations during
 *    the execution phase (e.g., via @ref OperationPipeline::applyOperations).
 * 3. **Display/Output:** Potentially converted or prepared for rendering or
 *    file export by other core or UI components.
 *
 * @warning The pixel data (`m_data`) is typically stored as `float32` values
 *          during processing to support High Dynamic Range (HDR) and preserve
 *          precision. Conversion to integer formats (e.g., `uint8`) for display
 *          or file output requires appropriate clamping (e.g., `[0.0f, 1.0f]`
 *          before multiplying by 255.0f) and rounding.
 *
 * @see SourceManager::getTile()
 * @see IOperation::execute()
 * @see OperationPipeline::applyOperations()
 * @see PixelFormat
 */

struct ImageRegion {

    /**
     * @brief X-coordinate of the top-left corner of this region in the full source image.
     *
     * This member indicates the horizontal offset of the top-left pixel of this
     * region relative to the full image from which it might have been extracted.
     * It is primarily informational and context-providing, especially for
     * operations that might need to know the absolute position within the source.
     */
    int m_x{0};

    /**
     * @brief Y-coordinate of the top-left corner of this region in the full source image.
     *
     * This member indicates the vertical offset of the top-left pixel of this
     * region relative to the full image from which it might have been extracted.
     * It is primarily informational and context-providing, especially for
     * operations that might need to know the absolute position within the source.
     */
    int m_y{0};

    /**
     * @brief Width of this image region in pixels.
     *
     * Must be a positive value.
     */
    int m_width{0};

    /**
     * @brief Height of this image region in pixels.
     *
     * Must be a positive value.
     */
    int m_height{0};

    /**
     * @brief Number of color channels per pixel.
     *
     * The number of channels must be consistent with the `m_format`.
     * Common values are 3 (e.g., for RGB) or 4 (e.g., for RGBA).
     * @see m_format
     */
    int m_channels{4};

    /**
     * @brief Format specifying how pixels are stored (channels and data type).
     *
     * Defines the pixel layout, including the number of channels and the data
     * type per channel (e.g., float32 vs uint8). This must be consistent with
     * `m_channels` and the actual content of `m_data`.
     * @see PixelFormat
     */
    PixelFormat m_format{PixelFormat::RGBA_F32};

    /**
     * @brief Pixel data (row-major layout)
     *
     * The pixel values are stored sequentially in row-major order.
     * The size of this vector must be exactly `m_width * m_height * m_channels`.
     * The data type of the values (e.g., float, uint8) is implicitly defined
     * by the `m_format` member.
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
     * @brief Validates the integrity of the ImageRegion's dimensions and data buffer size.
     *
     * Checks if the width, height, and channels are positive, and if the
     * size of the `m_data` vector matches the expected size calculated
     * from `m_width * m_height * m_channels`.
     *
     * @return true if all dimensions are positive and the data size is correct.
     * @return false otherwise (e.g., invalid dimensions or mismatched buffer size).
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
     * @brief Calculates the total size in bytes of the pixel data buffer.
     * @return The total size in bytes (`m_data.size() * sizeof(float)`).
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
     * @brief Provides safe, unchecked access to a specific pixel's channel value (non-const).
     *
     * This operator provides direct access to the pixel value at a given
     * (row, column, channel) index within the `m_data` buffer.
     *
     * @warning This function performs **no bounds checking**. The caller
     *          **must** ensure that the provided indices `y`, `x`, and `c`
     *          are within the valid range:
     *          - `0 <= y < m_height`
     *          - `0 <= x < m_width`
     *          - `0 <= c < m_channels`
     *          Failure to do so results in undefined behavior.
     *
     * @param y The row index (0 for the top row).
     * @param x The column index (0 for the leftmost column).
     * @param c The channel index (e.g., 0 for Red, 1 for Green, etc.).
     * @return A reference to the pixel value at the specified location.
     */
    [[nodiscard]] float& operator()(int y, int x, int c) noexcept {
        return m_data[static_cast<size_t>((y * m_width + x) * m_channels + c)];
    }

    /**
     * @brief Provides safe, unchecked access to a specific pixel's channel value (const).
     *
     * This const overload of the subscript operator provides read-only access
     * to the pixel value at a given (row, column, channel) index.
     *
     * @warning Like the non-const version, this function performs **no bounds checking**.
     *          The caller must ensure indices `y`, `x`, `c` are valid.
     *
     * @param y The row index.
     * @param x The column index.
     * @param c The channel index.
     * @return A const reference to the pixel value at the specified location.
     */
    [[nodiscard]] const float& operator()(int y, int x, int c) const noexcept {
        return m_data[static_cast<size_t>((y * m_width + x) * m_channels + c)];
    }
};

/**
 * @concept ValidImageRegion
 * @brief Checks if an ImageRegion object is valid according to its internal rules.
 *        This concept can be used to constrain functions that require a valid ImageRegion.
 */
template<typename T>
concept ValidImageRegion = requires(const T& t)
{
    requires std::same_as<T, ImageRegion>;
    { t.isValid() } -> std::same_as<bool>;
    // add others verfications
};

} // namespace Common

} // namespace CaptureMoment::Core
