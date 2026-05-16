#pragma once
/**
 * @file working_image_context.h
 * @brief Manages the lifecycle of a single WorkingImage instance and its associated data.
 *
 * This class encapsulates the creation, reuse, data ownership, and export of a WorkingImage.
 * It acts as the Single Source of Truth for the original and working pixel data.
 *
 * @see IWorkingImageHardware for the underlying hardware abstraction interface.
 * @see WorkingImageData for the underlying memory management.
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#include "common/types/image_types.h"
#include "common/image_region.h"
#include "image_processing/interfaces/i_working_image_hardware.h"
#include "image_processing/common/working_image_data.h"

#include <memory>

/**
 * @brief Manages the lifecycle of a single IWorkingImage instance.
 *
 * WorkingImageContext encapsulates WorkingImage creation and reuse logic.
 * Only one WorkingImage is needed at a time during image processing operations.
 *
 */

namespace CaptureMoment::Core {

namespace ImageProcessing {

/**
 * @brief Manages the lifecycle of a single IWorkingImage instance and its data.
 *
 * WorkingImageContext encapsulates WorkingImage creation and data ownership logic.
 * It ensures that only one WorkingImageData exists, preventing RAM duplication.
 */

class WorkingImageContext
{
public:
    /**
     * @brief Default constructor.
     *
     * Creates an empty context with no WorkingImage allocated.
     */
    WorkingImageContext() = default;

    /**
     * @brief Destructor.
     *
     * Automatically releases the managed WorkingImage.
     */
    ~WorkingImageContext() = default;

    // Non-copyable, movable
    WorkingImageContext(const WorkingImageContext&) = delete;
    WorkingImageContext& operator=(const WorkingImageContext&) = delete;
    WorkingImageContext(WorkingImageContext&&) = default;
    WorkingImageContext& operator=(WorkingImageContext&&) = default;

    /**
     * @brief Prepares the context with a new source image.
     *
     * Initializes the internal WorkingImageData (allocates RAM and stores original).
     * Then creates a new hardware backend (via Factory) and binds the data to it.
     *
     * @param original_tile The original image data to initialize the WorkingImage with.
     * @return true if preparation was successful, false if there was an error.
     */
    [[nodiscard]] bool prepare(std::unique_ptr<Common::ImageRegion> original_tile);

    /**
     * @brief Restores the internal working buffer to the original source data.
     *
     * This modifies the data owned by WorkingDataContext, which is automatically
     * reflected in the bound IWorkingImageHardware (since it works on views/spans).
     */
    void resetToOriginal();

    /**
     * @brief Checks if a WorkingImage is ready for use.
     *
     * @return true if prepare() has been called successfully, false otherwise.
     */
    [[nodiscard]] bool isReady() const noexcept;

    /**
     * @brief Exports the WorkingImage data to CPU.
     *
     * Calls exportToCPUCopy() on the managed WorkingImage to retrieve
     * the processed image data.
     *
     * @return ImageRegion containing the exported data.
     * @throws std::runtime_error if no WorkingImage is ready.
     */
    [[nodiscard]] std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError>
    getWorkingImageAsRegion() const;

    /**
     * @brief Gets the WorkingImage for processing.
     * @return Reference to the IWorkingImageHardware instance, or an error if not ready.
     */
    [[nodiscard]] std::shared_ptr<IWorkingImageHardware>
     getWorkingImage() noexcept;

    /**
     * @brief Downsamples the WorkingImage to the specified dimensions.
     *
     * This is used for generating thumbnails or previews.
     * The actual downsampling logic is delegated to the IWorkingImageHardware implementation.
     *
     * @param target_width The desired width of the downsampled image.
     * @param target_height The desired height of the downsampled image.
     * @return A new ImageRegion containing the downsampled image data, or an error if it fails.
     */
    [[nodiscard]] std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError>
    getDownsampled(Common::ImageDim target_width, Common::ImageDim target_height);

    /**
     * @brief Releases the managed WorkingImage.
     *
     * After calling this method, isReady() will return false
     * and prepare() must be called again before getWorkingImage().
     */
    void release() noexcept;

private:

    /** @brief The unique owner of the CPU RAM buffers (working + original). */
    std::unique_ptr<WorkingImageData> m_image_data;

    /** @brief 
     * Managed WorkingImage instance (hardware abstraction). 
     * */
    std::shared_ptr<IWorkingImageHardware> m_working_image;
};

} // namespace ImageProcessing
} // namespace CaptureMoment::Core
