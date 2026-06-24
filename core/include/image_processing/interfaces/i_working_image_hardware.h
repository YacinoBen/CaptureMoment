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
#include "common/types/image_types.h"

#include "image_processing/common/image_view.h"

#include <memory>
#include <expected>
#include <span>

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
     * @brief Binds a view of the image data to this hardware worker.
     *
     * @details
     * Default implementation stores the view and validates the working data span.
     * Derived classes (CPU, GPU) can override this to add hardware-specific initialization
     * (e.g., VRAM upload), but MUST call this base method first.
     *
     * @param view The non-owning view containing data pointers and geometry.
     * @return true if the working data span is valid.
     */
    [[nodiscard]] virtual bool bindView(const ImageView& view) {
        m_view_data_image = view;
        return !m_view_data_image.working_data.empty();
    }

    /**
     * @brief Exports current internal image data to a new CPU-based ImageRegion (Deep Copy).
     *
     * @details
     * Creates a deep copy of the internal buffer and returns it as a new ImageRegion.
     * The internal buffer remains valid and unchanged after this operation.
     *
     * **Performance Note:**
     * This method involves memory allocation and data copying. For large images,
     *
     * @return std::expected<std::unique_ptr<Common::ImageRegion>, std::error_code>
     *         Unique pointer to copied data on success.
     *
     * @see exportToCPUMove() For zero-copy transfer when working image can be invalidated.
     */
    [[nodiscard]] virtual std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError>
    getFullResImage() = 0;

    /**
     * @brief Exports a downscaled version of the image directly from GPU.
     *
     * @details
     * For GPU: Performs downsample on GPU, then transfers only the small result.
     * For CPU: Performs downsample on CPU.
     *
     * This is the preferred method for display purposes.
     */
    [[nodiscard]] virtual std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError>
    downsample(Common::ImageDim target_width, Common::ImageDim target_height) = 0;

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

    /**
     * @brief The image view bound to this hardware worker.
     */
    ImageView m_view_data_image;
};

} // namespace ImageProcessing

} // namespace CaptureMoment::Core
