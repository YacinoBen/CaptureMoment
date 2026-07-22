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
#include "types/image_types.h"

#include <vector>
#include <cstddef>
#include <concepts>
#include <span>
#include <cassert>
#include <limits>
#include <cstdint>
#include <mdspan>
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
    ImageCoord m_x{0};

    /**
     * @brief Y-coordinate of the top-left corner of this region in the full source image.
     */
    ImageCoord m_y{0};

    /**
     * @brief Width of this image region in pixels.
     */
    ImageDim m_width{0};

    /**
     * @brief Height of this image region in pixels.
     */
    ImageDim m_height{0};

    /**
     * @brief Number of color channels per pixel.
     */
    ImageChan m_channels{4};

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
    // Constructors
    // ============================================================

    /**
     * @brief Default constructor.
     * Creates an empty, invalid ImageRegion.
     */
    ImageRegion() = default;

    /**
     * @brief Constructs an ImageRegion and allocates uninitialized memory.
     * @details Useful for output buffers in algorithms.
     */
    ImageRegion(ImageDim w, ImageDim h, ImageChan ch, PixelFormat fmt = PixelFormat::RGBA_F32)
        : m_width(w)
        , m_height(h)
        , m_channels(ch)
        , m_format(fmt)
        , m_data(static_cast<std::size_t>(w) * h * ch) // Safe multiplication
    {}


    /**
     * @brief Constructs an ImageRegion by moving existing pixel data (Zero-Copy).
     *
     * @details
     * This constructor enables efficient transfer of pixel data ownership without
     * any memory copying. It is the preferred way to create an ImageRegion when
     * the source buffer is no longer needed.
     *
     * **Performance Benefits:**
     * - No memory allocation (takes ownership of existing vector)
     * - No data copying (O(1) operation)
     * - Ideal for pipeline data transfer between components
     *
     * @param data Rvalue reference to the pixel data vector. Ownership is transferred.
     * @param w Width in pixels.
     * @param h Height in pixels.
     * @param ch Number of color channels per pixel.
     * @param x X-coordinate offset in the source image.
     * @param y Y-coordinate offset in the source image.
     * @param fmt Pixel format of the data.
     *
     * @note The x and y coordinates default to (0, 0).
     * @note Format defaults to PixelFormat::RGBA_F32.
     */
    ImageRegion(std::vector<float>&& data, ImageDim w, ImageDim h, ImageChan ch,
                ImageCoord x = 0, ImageCoord y = 0, PixelFormat fmt = PixelFormat::RGBA_F32)
        : m_x(x)
        , m_y(y)
        , m_width(w)
        , m_height(h)
        , m_channels(ch)
        , m_format(fmt)
        , m_data(std::move(data))
    {}

    /**
     * @brief Constructs an ImageRegion by copying data from a span (Deep Copy).
     *
     * @details
     * This constructor is essential when working with non-owning views (std::span).
     * It allocates a new internal vector and copies the data from the span.
     * This is the standard way to export data from a WorkingImage (which works on spans)
     * back to a standalone ImageRegion.
     *
     * @param data_span Non-owning view over the pixel data to copy.
     * @param w Width in pixels.
     * @param h Height in pixels.
     * @param ch Number of color channels per pixel.
     * @param x X-coordinate offset in the source image.
     * @param y Y-coordinate offset in the source image.
     * @param fmt Pixel format of the data.
     *
     * @note The x and y coordinates default to (0, 0).
     * @note Format defaults to PixelFormat::RGBA_F32.
     */
    ImageRegion(std::span<const float> data_span, ImageDim w, ImageDim h, ImageChan ch,
                ImageCoord x = 0, ImageCoord y = 0, PixelFormat fmt = PixelFormat::RGBA_F32)
        : m_x(x)
        , m_y(y)
        , m_width(w)
        , m_height(h)
        , m_channels(ch)
        , m_format(fmt)
        , m_data(data_span.begin(), data_span.end())
    {}

    // ============================================================
    // Accessors & Utilities
    // ============================================================

    /*
     * @brief Accessor for width.
     * @return The width of the image region in pixels.
    */
    [[nodiscard]] constexpr ImageDim width() const noexcept { return m_width; }

    /*
     * @brief Accessor for height.
     * @return The height of the image region in pixels.
     */
   [[nodiscard]] constexpr ImageDim height() const noexcept { return m_height; }

    /*
     * @brief Accessor for channels.
     * @return The number of color channels per pixel.
     */
   [[nodiscard]] constexpr ImageChan channels() const noexcept { return m_channels; }

    /*
     * @brief Accessor for pixel format.
     * @return The pixel format of the image region.
     */
   [[nodiscard]] constexpr PixelFormat format() const noexcept { return m_format; }

    /*
     * @brief Accessor for x-coordinate.
     * @return The x-coordinate of the image region in the source image.
     */
   [[nodiscard]] constexpr ImageCoord x() const noexcept { return m_x; }

    /*
     * @brief Accessor for y-coordinate.
     * @return The y-coordinate of the image region in the source image.
     */
    [[nodiscard]] constexpr ImageCoord y() const noexcept { return m_y; }

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
        if (m_width == 0 || m_height == 0 || m_channels == 0) {
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

        // 3. Calculate pixel count safely (width * height)
        std::size_t pixel_count = 0;
        if (!safe_multiply(m_width, m_height, pixel_count)) {
            return false; // Overflow detected in width * height
        }

        // 4. Calculate total elements safely (pixel_count * channels)
        std::size_t expected_size = 0;
        if (!safe_multiply(pixel_count, m_channels, expected_size)) {
            return false; // Overflow detected in total elements
        }

        // 5. Consistency Check
        // Ensure the vector holds exactly the amount of data expected.
        return m_data.size() == expected_size;
    }

    /**
     * @brief Calculates the total size in bytes of the pixel data buffer.
     */
    [[nodiscard]] constexpr ImageSize sizeInBytes() const noexcept {
        return m_data.size() * sizeof(float);
    }

    /**
     * @brief Returns the total number of float elements in the pixel data buffer.
     *
     * @details This returns the actual size of the internal vector (`m_data.size()`),
     * which corresponds to `width * height * channels` if the region is valid.
     *
     * @return The total number of float elements.
     */
    [[nodiscard]] constexpr ImageSize getDataSize() const noexcept {
        return m_data.size();
    }

    /**
     * @brief Provides read-only access to the pixel data buffer as a std::span.
     */
    template <typename Self>
    [[nodiscard]] constexpr auto getBuffer(this Self&& self)  noexcept {
        return std::span{self.m_data};
    }

    /**
     * @brief Provides unchecked access to a specific pixel's channel value.
     *
     * Uses an assert in Debug mode to catch out-of-bounds errors early during development.
     * @note Uses C++23 deducing `this` to avoid duplicating const/non-const overloads.
     * @note Uses std::size_t to prevent signed/unsigned comparison issues.
     */

    template <typename Self>
    [[nodiscard]] auto& operator()(this Self&& self, std::size_t y, std::size_t x, std::size_t c) noexcept {
        assert(y < self.m_height);
        assert(x < self.m_width);
        assert(c < self.m_channels);
        const std::size_t idx = (y * self.m_width + x) * self.m_channels + c;
        return self.m_data[idx];
    }

    /**
     * @brief Returns std::mdspan (3D: Height, Width, Channels).
     * @note This requires C++23 for `this` parameter deduction.
     * @details Allows algorithms to access pixels as `mdspan(y, x, c)` directly
     *          without manual index calculation.
     */
    template <typename Self>
    [[nodiscard]] auto getMdSpan(this Self&& self) noexcept {
        using ValueType = std::remove_reference_t<decltype(self.m_data[0])>;
        return std::mdspan<ValueType, std::dextents<std::size_t, 3>>(
            self.m_data.data(), self.m_height, self.m_width, self.m_channels
        );
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
    { t.m_width } -> std::convertible_to<Common::ImageDim>;
    { t.m_height } -> std::convertible_to<Common::ImageDim>;
    { t.m_channels } -> std::convertible_to<Common::ImageChan>;

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
concept MutableImageLike = ImageLike<T> && requires(T& t)
{
    { t.getBuffer() } -> std::convertible_to<std::span<float>>;
};

} // namespace Common

} // namespace CaptureMoment::Core
