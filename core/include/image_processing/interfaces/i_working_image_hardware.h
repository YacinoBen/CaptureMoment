/**
 * @file i_working_image_hardware.h
 * @brief Abstract interface representing an image used as a working buffer, abstracting hardware location.
 *
 * @details
 * This interface abstracts the underlying hardware storage location (CPU RAM or GPU memory)
 * of the image data. It provides a unified set of operations that can be performed
 * on the image data regardless of its physical location.
 *
 * Implementations (e.g., WorkingImageCPU, WorkingImageGPU) handle the specifics
 * of memory allocation, data transfer, and operations based on their designated hardware location.
 *
 * **Error Handling:**
 * Methods use `std::expected` instead of exceptions or null pointers for robust error reporting.
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "common/types/memory_type.h"
#include "common/image_region.h"
#include "common/error_handling/core_error.h"
#include <memory>
#include <cstddef>
#include <utility>
#include <expected>

namespace CaptureMoment::Core {

namespace ImageProcessing {

/**
 * @interface IWorkingImageHardware
 * @brief Abstract interface representing an image used as a working buffer.
 *
 * @details
 * This interface defines the contract for all working image implementations.
 * It enforces strict ownership transfer semantics (unique pointers) and explicit error handling.
 */
class IWorkingImageHardware {
public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~IWorkingImageHardware() = default;

    /**
     * @brief Updates internal image data from a CPU-based ImageRegion.
     *
     * @param cpu_image The source image data.
     * @return std::expected<void, std::error_code> Success or error.
     */
    [[nodiscard]] virtual std::expected<void, ErrorHandling::CoreError>
    updateFromCPU(const Common::ImageRegion& cpu_image) = 0;

    /**
     * @brief Exports current internal image data to a new CPU-based ImageRegion.
     *
     * @details
     * Transfers ownership of the newly created ImageRegion to the caller.
     * The internal buffer remains valid for further operations.
     *
     * @return std::expected<std::unique_ptr<Common::ImageRegion>, std::error_code>
     *         Unique pointer to copied data on success.
     */
    [[nodiscard]] virtual std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError>
    exportToCPUCopy() = 0;

    /**
     * @brief Gets dimensions (width, height) of image data.
     * @return {width, height}. Returns {0, 0} if invalid.
     */
    [[nodiscard]] virtual std::pair<size_t, size_t> getSize() const = 0;

    /**
     * @brief Gets number of color channels.
     * @return Number of channels. Returns 0 if invalid.
     */
    [[nodiscard]] virtual size_t getChannels() const = 0;

    /**
     * @brief Gets total number of pixels.
     * @return width * height. Returns 0 if invalid.
     */
    [[nodiscard]] virtual size_t getPixelCount() const = 0;

    /**
     * @brief Gets total number of data elements (pixels * channels).
     * @return Total size. Returns 0 if invalid.
     */
    [[nodiscard]] virtual size_t getDataSize() const = 0;

    /**
     * @brief Checks if the image data is valid.
     * @return true if valid.
     */
    [[nodiscard]] virtual bool isValid() const = 0;

    /**
     * @brief Gets the memory type where data resides.
     * @return MemoryType enum.
     */
    [[nodiscard]] virtual Common::MemoryType getMemoryType() const = 0;

protected:
    /**
     * @brief Protected constructor to enforce abstract nature.
     */
    IWorkingImageHardware() = default;
};

} // namespace ImageProcessing

} // namespace CaptureMoment::Core
