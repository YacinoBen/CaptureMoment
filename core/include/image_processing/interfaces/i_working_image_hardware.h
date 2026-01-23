/**
 * @file i_working_image_hardware.h
 * @brief Abstract interface representing an image used as a working buffer, abstracting hardware location.
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "common/types/memory_type.h"
#include "common/image_region.h"

#include <memory>
#include <cstddef>

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
     * @brief Updates the internal image data of this object from a CPU-based ImageRegion.
     *
     * Depending on the concrete implementation (CPU or GPU), this method will
     * either copy data within CPU memory or transfer data from the provided
     * CPU memory source to the internal storage location (CPU or GPU).
     *
     * This method is typically used to initialize or update the working image
     * state from a source that resides in CPU memory (e.g., loaded via OIIO).
     *
     * @param[in] cpu_image The source image data residing in CPU memory.
     * @return true if the update operation was successful, false otherwise.
     */
    [[nodiscard]] virtual bool updateFromCPU(const Common::ImageRegion& cpu_image) = 0;

    /**
     * @brief Exports the current internal image data to a new CPU-based ImageRegion owned by the caller.
     *
     * This method creates a **new** ImageRegion instance on the heap and copies
     * the internal image data into it. The caller receives ownership of the returned
     * shared pointer and is responsible for its lifetime.
     * This involves a **deep copy** of the image data.
     *
     * This method is typically used when the caller needs an independent copy
     * of the image data that can be modified or held without affecting the
     * original IWorkingImageHardware object.
     *
     * @return A shared pointer to a **newly allocated** ImageRegion containing the copied image data.
     *         Returns nullptr on failure (allocation or copy error).
     */
    [[nodiscard]] virtual std::shared_ptr<Common::ImageRegion> exportToCPUCopy() = 0;

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
