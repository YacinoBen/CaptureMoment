/**
 * @file photo_engine.h
 * @brief Declaration of PhotoEngine class.
 *
 * @details
 * Central orchestrator for image loading, processing, and state management.
 * This class acts as the main entry point for the Core library. It bridges
 * the gap between I/O (SourceManager) and processing (StateImageManager).
 *
 * The engine handles the lifecycle of the image data, from loading the original
 * source to applying cumulative operations and exporting the result.
 *
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include "managers/source_manager.h"
#include "managers/state_image_manager.h"
#include "operations/operation_descriptor.h"
#include "common/image_region.h"
#include "common/error_handling/core_error.h"

#include <memory>
#include <string_view>
#include <vector>
#include <expected>

namespace CaptureMoment::Core {

namespace Engine {

/**
 * @class PhotoEngine
 * @brief Core engine orchestrating image loading and cumulative operation management.
 *
 * This class manages the dependencies between the SourceManager (file I/O) and
 * the StateImageManager (processing pipeline). It provides a simplified interface
 * for loading images, applying adjustments, and retrieving the processed result.
 */
class PhotoEngine
{
private:
    std::shared_ptr<Managers::SourceManager> m_source_manager;
    std::unique_ptr<Managers::StateImageManager> m_state_manager;

public:
    /**
     * @brief Constructs a PhotoEngine instance.
     *
     * Initializes internal managers and the operation factory.
     */
    PhotoEngine();

    /**
     * @brief Loads an image file and initializes the processing pipeline.
     *
     * This method performs the following steps:
     * 1. Loads the file from disk via SourceManager.
     * 2. Initializes the StateImageManager with the source image.
     * 3. Triggers the first processing update to prepare the working image.
     *
     * The call blocks until the initial processing is complete to ensure a valid
     * image is available immediately after the function returns.
     *
     * @param path Path to the image file.
     * @return `std::expected<void, CoreError>` indicating success or the specific error type.
     */
    [[nodiscard]] std::expected<void, ErrorHandling::CoreError> loadImage(std::string_view path);

    /**
     * @brief Commits the processed working image back to the source manager.
     *
     * Exports the current working image (potentially in GPU memory) to a CPU buffer
     * and writes it to the underlying SourceManager. This overwrites the original
     * image data in memory.
     *
     * @return `std::expected<void, CoreError>` indicating success or failure.
     */
    [[nodiscard]] std::expected<void, ErrorHandling::CoreError> commitWorkingImageToSource();

    /**
     * @brief Resets the working image to the original state.
     *
     * Clears all applied operations and reverts the working image to match the
     * original source image.
     */
    void resetWorkingImage();

    /**
     * @brief Gets the width of the loaded image.
     *
     * @return Image width in pixels, or 0 if no image is loaded.
     */
    [[nodiscard]] int width() const noexcept;

    /**
     * @brief Gets the height of the loaded image.
     *
     * @return Image height in pixels, or 0 if no image is loaded.
     */
    [[nodiscard]] int height() const noexcept;

    /**
     * @brief Gets the number of color channels.
     *
     * @return Number of channels (e.g., 4 for RGBA), or 0 if no image is loaded.
     */
    [[nodiscard]] int channels() const noexcept;

    /**
     * @brief Applies a cumulative list of operations.
     *
     * Replaces the current active operation list with the provided vector
     * and triggers an asynchronous pipeline update.
     *
     * This method is non-blocking. The processing happens in the background.
     * The caller can use `getWorkingImage` or `getWorkingImageAsRegion` to retrieve
     * the result when ready.
     *
     * @param ops Vector of OperationDescriptors defining the adjustments.
     */
    void applyOperations(const std::vector<Operations::OperationDescriptor>& ops);

    /**
     * @brief Gets the raw working image (Hardware Abstraction).
     *
     * Returns a shared pointer to the internal hardware-agnostic image.
     * This is useful if the caller needs to interface directly with the processing
     * backend.
     *
     * @return Shared pointer to the working image, or nullptr.
     */
    [[nodiscard]] std::shared_ptr<ImageProcessing::IWorkingImageHardware> getWorkingImage() const;

    /**
     * @brief Gets the working image as a CPU-based copy.
     *
     * This method exports the working image (which might reside in GPU memory)
     * into a standard CPU buffer (ImageRegion).
     *
     * This is the preferred method for external display or serialization logic,
     * as it ensures the data is available in CPU RAM.
     *
     * @return `std::expected` containing a unique pointer to the ImageRegion, or an error code.
     */
    [[nodiscard]] std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError> getWorkingImageAsRegion() const;
};

} // namespace Engine

} // namespace CaptureMoment::Core
