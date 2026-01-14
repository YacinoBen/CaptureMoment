/**
 * @file photo_engine.h
 * @brief Declaration of PhotoEngine class
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include "managers/source_manager.h"
#include "operations/operation_factory.h"
#include "operations/operation_pipeline.h"
#include "managers/state_image_manager.h"
#include "common/image_region.h"

#include <memory>
#include <string_view>
#include <vector>

#include <spdlog/spdlog.h>

namespace CaptureMoment::Core {

namespace Engine {

/**
 * @brief Central engine orchestrating image loading and cumulative operation management.
 *
 * The PhotoEngine acts as the main interface between the UI layer (Qt) and the core
 * image processing logic. It delegates the management of the cumulative image processing state
 * (the sequence of applied operations and the resulting image) to StateImageManager.
 *  It also manages the persistence of operations via FileSerializerManager.
 */
class PhotoEngine
{
private:
    /**
     * @brief Shared pointer to the manager responsible for loading and providing image data.
     */
    std::shared_ptr<Managers::SourceManager> m_source_manager;
    /**
     * @brief Shared pointer to the factory responsible for creating operation instances.
     */
    std::shared_ptr<Operations::OperationFactory> m_operation_factory;
    /**
     * @brief Shared pointer to the pipeline responsible for applying sequences of operations.
     */
    std::shared_ptr<Operations::OperationPipeline> m_operation_pipeline;

    /**
     * @brief Unique pointer to the manager responsible for the cumulative image processing state.
     * This manager handles the sequence of active operations and updates the working image accordingly.
     */
    std::unique_ptr<Managers::StateImageManager> m_state_manager;

public:
    /**
     * @brief Constructs a PhotoEngine instance.
     *
     * Initializes the engine with the necessary managers and factories.
     *
     * @param[in] source_manager A shared pointer to the SourceManager instance.
     * @param[in] operation_factory A shared pointer to the OperationFactory instance.
     * @param[in] operation_pipeline A shared pointer to the OperationPipeline instance.
     */
    PhotoEngine(
        std::shared_ptr<Managers::SourceManager> source_manager,
        std::shared_ptr<Operations::OperationFactory> operation_factory,
        std::shared_ptr<Operations::OperationPipeline> operation_pipeline
        );

    /**
     * @brief Loads an image file into the engine.
     *
     * Delegates the loading process to the internal SourceManager.
     * Initializes the StateImageManager with the loaded image path.
     *
     * @param[in] path The path to the image file to be loaded.
     * @return true if the image was loaded successfully, false otherwise.
     */
    [[nodiscard]] bool loadImage(std::string_view path);

    /**
     * @brief Commits the current working image back to the source image.
     *
     * This method should be called when the user wants to save the changes permanently.
     * It replaces the original image in m_source_manager with the current working image
     * managed by StateImageManager.
     *
     * @return true if the working image was successfully committed to the source manager, false otherwise.
     */
    [[nodiscard]] bool commitWorkingImageToSource();

    /**
     * @brief Resets the working image to the original image loaded from the source manager.
     *
     * This method delegates the reset to the internal StateImageManager,
     * effectively undoing all applied operations.
     */
    void resetWorkingImage();

    /**
     * @brief Gets the width of the currently loaded image.
     *
     * Delegates the call to the internal SourceManager.
     *
     * @return The width in pixels, or 0 if no image is loaded.
     */
    [[nodiscard]] int width() const noexcept;

    /**
     * @brief Gets the height of the currently loaded image.
     *
     * Delegates the call to the internal SourceManager.
     *
     * @return The height in pixels, or 0 if no image is loaded.
     */
    [[nodiscard]] int height() const noexcept;

    /**
     * @brief Gets the number of channels of the currently loaded image.
     *
     * Delegates the call to the internal SourceManager.
     *
     * @return The number of channels (e.g., 3 for RGB, 4 for RGBA), or 0 if no image is loaded.
     */
    [[nodiscard]] int channels() const noexcept;

    /**
     * @brief Applies a sequence of operations cumulatively to the working image.
     *
     * This method delegates the application of the operations to the internal StateImageManager.
     * The StateImageManager handles the update asynchronously.
     *
     * @param ops The vector of OperationDescriptors to apply.
     */
    void applyOperations(const std::vector<Operations::OperationDescriptor>& ops);

    /**
     * @brief Gets the current working image.
     *
     * Delegates the call to the internal StateImageManager to get the image
     * reflecting the currently applied operations.
     *
     * @return Shared pointer to the current working ImageRegion managed by StateImageManager.
     */
    [[nodiscard]] std::shared_ptr<Common::ImageRegion> getWorkingImage() const;
};

} // namespace Engine

} // namespace CaptureMoment::Core
