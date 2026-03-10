/**
 * @file working_image_data.h
 * @brief Base class providing raw buffer storage and metadata for all working image implementations.
 *
 * @details
 * This class serves as a foundation for all WorkingImage implementations, regardless of
 * their processing backend (CPU/GPU) or library (Halide/OpenCV/etc.). It encapsulates
 * the common data storage and metadata required for image processing operations.
 *
 * **Architecture Role:**
 * This class is designed as a mixin/base class that provides:
 * - Raw pixel data storage using `std::unique_ptr<float[]>` for efficient memory management
 * - Image metadata (width, height, channels, validity state)
 * - A shared initialization helper that handles allocation and copying from ImageRegion
 *
 * **Memory Management:**
 * The class uses `std::unique_ptr<float[]>` instead of `std::vector<float>` to enable
 * `std::make_unique_for_overwrite`, which skips zero-initialization. This significantly
 * reduces allocation time for large buffers (e.g., 400MB+ images).
 *
 * **Inheritance Hierarchy:**
 * @code
 * WorkingImageData (this class)
 *         │
 *         ├── WorkingImageCPU : WorkingImageData
 *         │       │
 *         │       └── WorkingImageCPU_Halide
 *         │
 *         └── WorkingImageGPU : WorkingImageData
 *                 │
 *                 └── WorkingImageGPU_Halide
 * @endcode
 *
 * **Error Handling:**
 * Methods use `std::expected` instead of exceptions for robust error reporting.
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "common/image_region.h"
#include "common/error_handling/core_error.h"
#include "common/types/image_types.h"

#include <memory>
#include <expected>
#include <cstddef>

namespace CaptureMoment::Core {

namespace ImageProcessing {

/**
 * @class WorkingImageData
 * @brief Base class providing raw buffer storage and metadata for working images.
 *
 * @details
 * This class provides the fundamental data storage infrastructure shared by all
 * WorkingImage implementations. It owns the raw pixel buffer and associated metadata,
 * making it available to derived classes through protected members.
 *
 * **Key Features:**
 * - Zero-initialization-free allocation for performance
 * - Automatic metadata population during initialization
 * - Shared allocation/copy logic to eliminate code duplication
 */
class WorkingImageData {
protected:
    /**
     * @brief Internal storage for image pixel data.
     *
     * @details
     * Uses `std::unique_ptr<float[]>` with`std::make_unique_for_overwrite`, which allocates memory WITHOUT zero-initialization.
     * This optimization can save hundreds of milliseconds for large images.
     *
     * **Ownership:** This pointer owns the allocated memory. Derived classes can read
     * and write directly to this buffer.
     *
     * **Lifecycle:** Allocated during `initializeAndCopyFrom()`, deallocated when
     * the object is destroyed or when `exportToCPUMove()` is called.
     */
    std::unique_ptr<float[]> m_data;

    /**
     * @brief Number of float elements stored in m_data.
     *
     * @details
     * Equal to `m_width * m_height * m_channels`. Required because `unique_ptr`
     * does not store size information.
     *
     * **Value:** 0 when no data is allocated; > 0 when valid data exists.
     */
    Common::ImageSize m_data_size{0};

    /**
     * @brief Image width in pixels.
     *
     * @details
     * Populated from ImageRegion during `initializeAndCopyFrom()`.
     * Returns to 0 when the working image is invalidated (e.g., after `exportToCPUMove()`).
     */
    Common::ImageDim m_width{0};

    /**
     * @brief Image height in pixels.
     *
     * @details
     * Returns to 0 when the working image is invalidated.
     */
    Common::ImageDim m_height{0};

    /**
     * @brief Number of color channels per pixel.
     *
     * @details
     * Typical values: 3 (RGB), 4 (RGBA).
     */
    Common::ImageChan m_channels{0};

    /**
     * @brief Validity flag indicating whether the buffer contains usable data.
     *
     * @details
     * - `true`: Buffer is allocated and contains valid image data
     * - `false`: Buffer is empty, deallocated, or moved
     *
     * **State Transitions:**
     * - Set to `true` by `initializeAndCopyFrom()`
     * - Set to `false` by `exportToCPUMove()` or when initialization fails
     */
    bool m_valid{false};

    /**
     * @brief Allocates buffer and copies pixel data from an ImageRegion.
     *
     * @details
     * This method performs the following operations:
     * 1. Validates the input ImageRegion
     * 2. Calculates required buffer size (width × height × channels)
     * 3. Allocates memory WITHOUT zero-initialization if size changed
     * 4. Copies pixel data from ImageRegion to internal buffer
     * 5. Populates metadata (width, height, channels, valid flag)
     *
     * **Performance:**
     * - Uses `std::make_unique_for_overwrite` to skip zero-initialization
     * - Reuses existing buffer if size matches (avoids reallocation)
     * - Single `memcpy` for data transfer
     *
     * **Error Conditions:**
     * - Returns `InvalidImageRegion` if input ImageRegion is not valid
     * - Returns `AllocationFailed` if memory allocation fails
     *
     * @param cpu_image The source ImageRegion containing pixel data and metadata.
     *                  Must be valid (isValid() returns true).
     *
     * @return `std::expected<void, CoreError>`:
     *         - Success: empty void result
     *         - Failure: CoreError indicating the failure reason
     *
     * @pre `cpu_image.isValid() == true`
     * @post On success: `m_valid == true`, `m_data` contains pixel data,
     *       metadata fields are populated
     * @post On failure: State unchanged
     *
     * @note This method is protected and intended to be called by derived classes
     *       during their `updateFromCPU()` implementation.
     */
    [[nodiscard]] std::expected<void, ErrorHandling::CoreError>
    initializeData(const Common::ImageRegion& cpu_image);

    /**
     * @brief Returns a non-owning span over the pixel data.
     */
    [[nodiscard]] std::span<float> getDataSpan() noexcept {
        return std::span<float>(m_data.get(), m_data_size);
    }

    /**
     * @brief Returns a const non-owning span over the pixel data.
     */
    [[nodiscard]] std::span<const float> getDataSpan() const noexcept {
        return std::span<const float>(m_data.get(), m_data_size);
    }

    /**
     * @brief Protected default constructor.
     *
     * @details
     * Prevents direct instantiation of this base class. Only derived classes
     * can construct WorkingImageData as part of their inheritance chain.
     */
    WorkingImageData() = default;
};

} // namespace ImageProcessing

} // namespace CaptureMoment::Core
