#pragma once
/**
 * @file working_image_context.h
 * @brief Manages the lifecycle of a single WorkingImage instance.
 * 
 * This class encapsulates the creation, reuse, and export of a WorkingImage.
 * It ensures that only one WorkingImage is active at a time, optimizing resource usage.
 * 
 * The context can prepare a WorkingImage with specified dimensions, check if it's ready,
 * export its data to CPU, and release it when done.
 * 
 * @see IWorkingImageHardware for the underlying hardware abstraction interface.
 * 
 * @author CaptureMoment Team
 * @date 2026
 */
#include "common/types/image_types.h"
#include "common/image_region.h"
#include "image_processing/interfaces/i_working_image_hardware.h"

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
     * @brief Prepares a WorkingImage with specified dimensions.
     * 
     * If a WorkingImage already exists with the same dimensions, it is reused.
     * Otherwise, a new WorkingImage is created.
     * 
     * @param original_tile The original image data to initialize the WorkingImage with.
     * @return true if preparation was successful, false if there was an error (e.g. creation failed).
     */
    [[nodiscard]] bool prepare(std::unique_ptr<Common::ImageRegion>&& original_tile);

    /**
     * @brief Updates the existing WorkingImage with new image data.
     * 
     * This is used when we want to reuse the same WorkingImage instance but with different content.
     * 
     * @param original_tile The new image data to update the WorkingImage with.
     * @return true if the update was successful, false if there was an error (e.g. no existing WorkingImage).
     */

    [[nodiscard]] bool update(const Common::ImageRegion& original_tile);

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
    downsample(Common::ImageDim target_width, Common::ImageDim target_height);
    /**
     * @brief Releases the managed WorkingImage.
     * 
     * After calling this method, isReady() will return false
     * and prepare() must be called again before getWorkingImage().
     */
    void release() noexcept;

private:
    /** @brief 
     * Managed WorkingImage instance (hardware abstraction). 
     * */
    std::shared_ptr<IWorkingImageHardware> m_working_image;
};

} // namespace ImageProcessing
} // namespace CaptureMoment::Core
