/**
 * @file i_working_image_hardware.h
 * @brief Abstract interface representing an image used as a working buffer, abstracting hardware location.
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "common/types/memory_type.h"
#include "common/image_region.h"
#include "common/error_handling/core_error.h"

#include <memory>
#include <cstddef>
#include <expected>

namespace CaptureMoment::Core {

namespace ImageProcessing {

/**
 * @brief Abstract interface representing an image used as a working buffer, abstracting hardware location.
 *
 * This interface abstracts the underlying hardware storage location (CPU RAM or GPU memory)
 * of the image data. It provides a unified set of operations that can be performed
 * on the image data regardless of its physical location. This allows higher-level
 * components (like StateImageManager, OperationPipeline) to interact with the image
 * without needing to know the specific details of how and where the data is stored,
 * enabling seamless switching between CPU and GPU backends.
 *
 * Implementations of this interface (e.g., WorkingImageCPU, WorkingImageGPU)
 * will handle the specifics of memory allocation, data transfer, and operations
 * based on their designated hardware location.
 *
 * The primary purpose is to hold the current state of the image undergoing
 * processing operations managed by the core engine.
 */
class IWorkingImageHardware {
public:
    /**
     * @brief Virtual destructor for safe inheritance and polymorphic deletion.
     */
    virtual ~IWorkingImageHardware() = default;

    /**
     * @brief Updates the internal image data from a CPU-based ImageRegion.
     *
     * @param cpu_image The source image data.
     * @return std::expected<void, std::error_code>.
     *         Returns {} (void) on success, or an error code on failure.
     */
    [[nodiscard]] virtual std::expected<void, std::error_code>
    updateFromCPU(const Common::ImageRegion& cpu_image) = 0;

    /**
     * @brief Exports the current internal image data to a new CPU-based ImageRegion.
     *
     * Uses std::expected to return either the result or an error,
     * avoiding the "Error by nullptr" ambiguity.
     *
     * @return std::expected<std::unique_ptr<Common::ImageRegion>, std::error_code>.
     *         Returns the unique_ptr on success, or an error code on failure.
     */
    [[nodiscard]] virtual std::expected<std::unique_ptr<Common::ImageRegion>, std::error_code>
    exportToCPUCopy() = 0;

    /**
     * @brief Gets the dimensions (width, height) of the image data.
     *
     * @return A pair containing the width (first) and height (second) of the image.
     *         Returns {0, 0} if the image size is not defined or invalid.
     */
    [[nodiscard]] virtual std::pair<size_t, size_t> getSize() const = 0;

    /**
     * @brief Gets the number of color channels of the image data.
     *
     * @return The number of channels (e.g., 3 for RGB, 4 for RGBA). Returns 0
     *         if the channel count is not defined or invalid.
     */
    [[nodiscard]] virtual size_t getChannels() const = 0;

    /**
     * @brief Gets the total number of pixels in the image data.
     *
     * @return The product of width and height. Returns 0 if size is invalid.
     */
    [[nodiscard]] virtual size_t getPixelCount() const = 0;

    /**
     * @brief Gets the total number of data elements (pixels * channels) in the image data.
     *
     * @return The product of pixel count and channel count. Returns 0 if size
     *         or channel count is invalid.
     */
    [[nodiscard]] virtual size_t getDataSize() const = 0;

    /**
     * @brief Checks if the image data within this working image is in a valid state.
     *
     * This indicates whether the object holds properly allocated and initialized
     * image data ready for operations.
     *
     * @return true if the image data is valid, false otherwise.
     */
    [[nodiscard]] virtual bool isValid() const = 0;

    /**
     * @brief Gets the memory type (CPU RAM or GPU memory) where the image data resides.
     *
     * @return An enum value indicating the current storage memory type of the image data.
     */
    [[nodiscard]] virtual Common::MemoryType getMemoryType() const = 0;

    // Potentially, future methods could be added here if needed by multiple backends.
    // For example, a method to get raw data pointers (carefully!), or methods related
    // to specific buffer management, but these are kept minimal for now.

protected:
    // Protected constructor to enforce abstract nature.
    IWorkingImageHardware() = default;
};

} // namespace ImageProcessing

} // namespace CaptureMoment::Core
