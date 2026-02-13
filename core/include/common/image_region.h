/**
 * @file image_region.h
 * @brief Defines the ImageRegion structure for representing pixel buffers in the CaptureMoment Core library.
 *
 * This header provides the @ref CaptureMoment::Core::Common::ImageRegion struct,
 * which encapsulates the raw pixel data for a rectangular region of an image.
 *
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include "pixel_format.h"
#include <vector>
#include <cstddef>
#include <concepts>
#include <span>
#include <cassert>
#include <limits>
#include <cstdint>

namespace CaptureMoment::Core {

namespace Common {

/**
 * @struct ImageRegion
 * @brief Represents a rectangular region of an image and its pixel data.
 *
 * This structure holds the raw pixel values for a specified rectangular area
 * of an image. It defines the spatial location (`m_x`, `m_y`), dimensions
 * (`m_width`, `m_height`), color format (`m_channels`, `m_format`), and the
 * actual pixel data (`m_data`).
 *
 * @par Memory Layout
 * Row-major order: `data[(y * width + x) * channels + c]`.
 * Total number of elements: `getDataSize() = width * height * channels`.
 *
 * @par Design Choice (Value Type)
 * ImageRegion is designed as a POD-like struct for efficient copying/moving by value
 * (e.g., returning from a SourceManager). However, deep copies of m_data are expensive.
 * Prefer passing by `std::span<float>` in algorithms that only read data.
 */
struct ImageRegion {

    // ============================================================
    // Dimensions & Meta-data
    // ============================================================

    /**
     * @brief X-coordinate of the top-left corner of this region in the full source image.
     */
    std::int32_t m_x{0};

    /**
     * @brief Y-coordinate of the top-left corner of this region in the full source image.
     */
    std::int32_t m_y{0};

    /**
     * @brief Width of this image region in pixels.
     */
    std::int32_t m_width{0};

    /**
     * @brief Height of this image region in pixels.
     */
    std::int32_t m_height{0};

    /**
     * @brief Number of color channels per pixel.
     */
    std::int32_t m_channels{4};

    /**
     * @brief Format specifying how pixels are stored.
     */
    PixelFormat m_format{PixelFormat::RGBA_F32};

    /**
     * @brief Pixel data (row-major layout).
     *
     * Stored as float32 to support HDR.
     */
    std::vector<float> m_data;

    // ============================================================
    // Accessors & Utilities
    // ============================================================

    /**
     * @brief Validates the integrity of the ImageRegion (Overflow-Safe Version).
     *
     * Improvements over basic version:
     * 1. Detects unsigned integer overflow during size calculation.
     * 2. Validates that the expected size matches the actual data vector size.
     * 3. Rejects absurd channel counts or zero dimensions.
     *
     * @return true if dimensions are valid and data matches the expected size, false otherwise.
     */
    [[nodiscard]] constexpr bool isValid() const noexcept
    {
        // 1. Basic sanity checks
        if (m_width <= 0 || m_height <= 0 || m_channels <= 0) {
            return false;
        }

        // Optional: Reject unreasonable channel counts (e.g. > 8 for typical RGB/CMYK)
        // Prevents logic errors where dimensions are small but channels are huge causing overflow.
        if (m_channels > 8) {
            return false;
        }

        // 2. Safe Calculation Helper (Prevents Overflow)
        // Using a lambda to keep the logic local and constexpr-friendly.
        auto safe_multiply = [](std::size_t a, std::size_t b, std::size_t& out_result) constexpr -> bool {
            if (a != 0 && b > std::numeric_limits<std::size_t>::max() / a) {
                // b > MAX / a  implies  a * b > MAX  (Integer Overflow)
                return false;
            }
            out_result = a * b;
            return true;
        };

        // Cast to std::size_t to work with unsigned arithmetic
        const std::size_t w = static_cast<std::size_t>(m_width);
        const std::size_t h = static_cast<std::size_t>(m_height);
        const std::size_t c = static_cast<std::size_t>(m_channels);

        // 3. Calculate pixel count safely (width * height)
        std::size_t pixel_count = 0;
        if (!safe_multiply(w, h, pixel_count)) {
            return false; // Overflow detected in width * height
        }

        // 4. Calculate total elements safely (pixel_count * channels)
        std::size_t expected_size = 0;
        if (!safe_multiply(pixel_count, c, expected_size)) {
            return false; // Overflow detected in total elements
        }

        // 5. Consistency Check
        // Ensure the vector holds exactly the amount of data expected.
        return m_data.size() == expected_size;
    }

    /**
     * @brief Calculates the total size in bytes of the pixel data buffer.
     */
    [[nodiscard]] constexpr std::size_t sizeInBytes() const noexcept {
        return m_data.size() * sizeof(float);
    }

    /**
     * @brief Calculates the total number of data elements (pixels * channels) in the pixel data buffer.
     * @return The total number of float elements (m_width * m_height * m_channels).
     */
    [[nodiscard]] constexpr std::size_t getDataSize() const noexcept {
        return m_data.size();
    }

    /**
     * @brief Returns a non-owning std::span over the pixel data.
     *
     * This is the preferred method to pass the image data to algorithms
     * or external APIs (like Halide) without copying the vector.
     *
     * @return std::span<float> view of the internal buffer.
     */
    [[nodiscard]] std::span<float> getBuffer() noexcept {
        return m_data;
    }

    /**
     * @brief Returns a const non-owning std::span over the pixel data.
     */
    [[nodiscard]] std::span<const float> getBuffer() const noexcept {
        return m_data;
    }

    /**
     * @brief Provides unchecked access to a specific pixel's channel value.
     *
     * Uses an assert in Debug mode to catch out-of-bounds errors early during development.
     *
     * @warning Release mode performs no bounds checking for maximum performance.
     */
    [[nodiscard]] float& operator()(int y, int x, int c) noexcept {
        assert(y >= 0 && y < m_height);
        assert(x >= 0 && x < m_width);
        assert(c >= 0 && c < m_channels);
        // Cast to size_t safely before arithmetic
        const std::size_t idx = (static_cast<std::size_t>(y) * m_width + x) * m_channels + c;
        return m_data[idx];
    }

    /**
     * @brief Const overload of operator().
     */
    [[nodiscard]] const float& operator()(int y, int x, int c) const noexcept {
        assert(y >= 0 && y < m_height);
        assert(x >= 0 && x < m_width);
        assert(c >= 0 && c < m_channels);
        const std::size_t idx = (static_cast<std::size_t>(y) * m_width + x) * m_channels + c;
        return m_data[idx];
    }
};

// ============================================================
// C++23 Concepts
// ============================================================

/**
 * @concept ImageLike
 * @brief A concept that constrains a type to behave like an image container.
 *
 * It allows any struct that
 * provides the necessary interface (width, height, data access) to be used in
 * generic image processing algorithms. This is crucial for polymorphism without
 * inheritance overhead.
 */
template<typename T>
concept ImageLike = requires(const T& t)
{
    // Must have dimensions
    { t.m_width } -> std::convertible_to<int>;
    { t.m_height } -> std::convertible_to<int>;
    { t.m_channels } -> std::convertible_to<int>;

    // Must have a validity check
    { t.isValid() } -> std::same_as<bool>;

    // Must support read-only access via getBuffer() or similar
    // (We check if it provides a view into contiguous float data)
    { t.getBuffer() } -> std::convertible_to<std::span<const float>>;
};

/**
 * @concept MutableImageLike
 * @brief Extends ImageLike to require read/write access.
 */
template<typename T>
concept MutableImageLike = ImageLike<T> && requires(T t)
{
    { t.getBuffer() } -> std::convertible_to<std::span<float>>;
};

} // namespace Common

} // namespace CaptureMoment::Core
