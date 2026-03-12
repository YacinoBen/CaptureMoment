/**
 * @file working_image_halide.h
 * @brief Base class for Halide-based working image implementations.
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include <vector>
#include <cstddef>
#include <span>

#include "common/image_region.h"
#include "common/types/image_types.h"

#include "Halide.h"

namespace CaptureMoment::Core {

namespace ImageProcessing {

/**
 * @brief Base class providing common Halide buffer functionality for image processing.
 *
 * This abstract base class manages the shared Halide buffer infrastructure
 * used by both CPU and GPU working image implementations. It provides the
 * common interface for direct buffer access during pipeline execution.
 */
class WorkingImageHalide {
public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~WorkingImageHalide() = default;

    /**
     * @brief Gets a reference to the internal Halide buffer managed by this object.
     *
     * This method provides direct access to the underlying Halide::Buffer<float>
     * that holds the image data. The buffer shares memory with the internal m_data vector,
     * allowing in-place modifications during pipeline execution. This enables the pipeline
     * to operate directly on the managed data without requiring intermediate copies.
     *
     * @return A Halide::Buffer<float> object that references the internal data storage.
     * The returned buffer points to the same memory location as m_data.
     */
    [[nodiscard]] Halide::Buffer<float> getHalideBuffer() const { return m_halide_buffer; };

protected:
    /**
     * @brief Internal Halide buffer holding the image data.
     * Managed directly by this object. Halide::Buffer handles its own memory allocation/deallocation.
     */
    Halide::Buffer<float> m_halide_buffer;

    /**
     * @brief Protected constructor to prevent direct instantiation.
     */
    WorkingImageHalide() = default;

    /**
     * @brief Creates a Halide buffer view over existing pixel data.
     *
     * @details
     * Creates a zero-copy Halide::Buffer that references the provided
     * pixel data span. This method should be called by derived classes after
     * they have allocated and populated their data buffer.
     *
     * **Safety:**
     * Uses `std::span<float>` instead of raw pointer for bounds safety
     * and compatibility with any contiguous container.
     *
     * @param data Span over pixel data (must remain valid during buffer lifetime).
     * @param width Image width in pixels.
     * @param height Image height in pixels.
     * @param channels Number of color channels.
     *
     * @pre !data.empty()
     * @post m_halide_buffer references the provided data
     */
    void initializeHalide(std::span<float> data, Common::ImageDim width, Common::ImageDim height, Common::ImageChan channels);

    /**
     * @brief
     * @return {width, height} in pixels, or {0, 0} if buffer undefined. */
    [[nodiscard]] std::pair<Common::ImageDim, Common::ImageDim> getSizeByHalide() const noexcept;

    /**
     * @brief Gets the number of color channels from the Halide buffer.
     * @return Channel count (e.g., 3 for RGB, 4 for RGBA), or 0 if buffer undefined.
     */
    [[nodiscard]] Common::ImageChan getChannelsByHalide() const noexcept;

    /**
     * @brief Gets the total pixel count from the Halide buffer.
     * @return width × height, or 0 if buffer undefined.
     */
    [[nodiscard]] Common::ImageSize getPixelCountByHalide() const noexcept;

    /**
     * @brief Gets the total data element count from the Halide buffer.
     * @return width × height × channels (total float elements), or 0 if buffer undefined.
     */
    [[nodiscard]] Common::ImageSize getDataSizeByHalide() const noexcept;

    /**
     * @brief Checks if the Halide buffer is defined.
     * @return true if m_halide_buffer.defined() returns true.
     */
    [[nodiscard]] bool isHalideBufferValid() const noexcept;
};

} // namespace ImageProcessing

} // namespace CaptureMoment::Core
