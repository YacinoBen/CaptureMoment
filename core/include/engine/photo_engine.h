/**
 * @file i_processing_task.h
 * @brief Declaration of PhotoEngine class
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <memory>

#include "managers/source_manager.h"
#include "operations/operation_factory.h"
#include "domain/i_processing_backend.h"

namespace CaptureMoment {

/**
 * @brief Central engine orchestrating image loading, processing task creation, and submission.
 *
 * The PhotoEngine acts as the main interface between the UI layer (Qt) and the core
 * image processing logic (PipelineEngine, SourceManager, OperationFactory).
 * It manages the state of the currently loaded image and handles the execution
 * of processing tasks, ensuring a sequential and coherent workflow.
 * This class implements the IProcessingBackend interface to standardize task management.
 */
class PhotoEngine : public IProcessingBackend {
private:
    /**
     * @brief Shared pointer to the manager responsible for loading and providing image data.
     */
    std::shared_ptr<SourceManager> m_source_manager;
    /**
     * @brief Shared pointer to the factory responsible for creating operation instances.
     */
    std::shared_ptr<OperationFactory> m_operation_factory;

public:
    /**
     * @brief Constructs a PhotoEngine instance.
     *
     * Initializes the engine with the necessary managers and factories.
     *
     * @param[in] source_manager A shared pointer to the SourceManager instance.
     * @param[in] operation_factory A shared pointer to the OperationFactory instance.
     */
    PhotoEngine(
        std::shared_ptr<SourceManager> source_manager,
        std::shared_ptr<OperationFactory> operation_factory
    );

    /**
     * @brief Loads an image file into the engine.
     *
     * Delegates the loading process to the internal SourceManager.
     *
     * @param[in] path The path to the image file to be loaded.
     * @return true if the image was loaded successfully, false otherwise.
     */
    bool loadImage(std::string_view path);

    /**
     * @brief Creates a new processing task.
     *
     * This method is part of the IProcessingBackend interface.
     * It should prepare and return a new IProcessingTask object,
     * although the specific parameters (like operations, region) might need
     * to be provided differently depending on the implementation details
     * (e.g., maybe the task itself holds this data or it's passed later).
     * For now, this signature matches the IProcessingBackend interface,
     * but might require refinement based on how tasks receive their input data
     * and operation list.
     *
     * @return A shared pointer to the newly created IProcessingTask.
     */
    std::shared_ptr<IProcessingTask> createTask(   
        const std::vector<OperationDescriptor>& ops,
        int x, int y, int width, int height) override;

    /**
     * @brief Submits a processing task for execution.
     *
     * This method is part of the IProcessingBackend interface.
     * It handles the execution of the given task. In the current design,
     * this likely means executing the task synchronously and waiting for its completion
     * before returning, ensuring only one task modifies the image state at a time.
     *
     * @param[in] task A shared pointer to the IProcessingTask to be executed.
     * @return true if the task was submitted and completed successfully, false otherwise.
     */
    bool submit(std::shared_ptr<IProcessingTask> task) override;

    /**
     * @brief Commits the result of a completed task to the source image.
     *
     * After a task has been executed successfully, this method takes its result
     * and applies it back to the internal SourceManager's image buffer,
     * making the changes permanent. This step is separated from `submit` to allow
     * previewing results without altering the original image.
     *
     * @param[in] task A shared pointer to the completed IProcessingTask whose
     *                 result needs to be committed.
     * @return true if the result was successfully committed, false otherwise.
     */
    bool commitResult(const std::shared_ptr<IProcessingTask>& task);

    /**
     * @brief Gets the width of the currently loaded image.
     *
     * Delegates the call to the internal SourceManager.
     *
     * @return The width in pixels, or 0 if no image is loaded.
     */
    int width() const noexcept;

    /**
     * @brief Gets the height of the currently loaded image.
     *
     * Delegates the call to the internal SourceManager.
     *
     * @return The height in pixels, or 0 if no image is loaded.
     */
    int height() const noexcept;

    /**
     * @brief Gets the number of channels of the currently loaded image.
     *
     * Delegates the call to the internal SourceManager.
     *
     * @return The number of channels (e.g., 3 for RGB, 4 for RGBA), or 0 if no image is loaded.
     */
    int channels() const noexcept;
};

} // namespace CaptureMoment
