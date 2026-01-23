/**
 * @file working_image_halide.h
 * @brief Base class for Halide-based working image implementations.
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include <vector>

#include "common/image_region.h"

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
     * @brief Internal storage for image data to be shared between operations.
     *
     * This vector holds the actual pixel data in float format that can be accessed
     * by both Halide operations and CPU-based processing. It serves as the backing
     * store for the Halide::Buffer, allowing efficient in-place modifications
     * without unnecessary data copying between operations.
     * The vector is managed internally and updated through updateFromCPU operations.
     */
    std::vector<float> m_data;

    /**
     * @brief Protected constructor to prevent direct instantiation.
     */
    WorkingImageHalide() = default;

    /**
     * @brief Initializes the internal Halide buffer to reference the current m_data vector.
     *
     * This private helper method creates a Halide::Buffer<float> that points to
     * the internal m_data vector, establishing the connection between the managed
     * data storage and the Halide processing engine. The buffer shares the same
     * memory as m_data, enabling in-place modifications during pipeline execution.
     * This method should only be called after m_data has been properly populated
     * via updateFromCPU operations to ensure the buffer points to valid data.
     *
     * @param cpu_image The ImageRegion containing the dimensional metadata (width, height, channels)
     * that defines the buffer layout. The actual pixel data comes from m_data.
     */
    void initializeHalide(const Common::ImageRegion& working_image);
};

} // namespace ImageProcessing

} // namespace CaptureMoment::Core
