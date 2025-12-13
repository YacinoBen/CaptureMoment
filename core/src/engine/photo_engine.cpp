/**
 * @file photo_engine.cpp
 * @brief Implementation of PhotoEngine
 * @author CaptureMoment Team
 * @date 2025
 */

#include "engine/photo_engine.h"
#include "engine/photo_task.h"

namespace CaptureMoment {

    // Constructor: Initializes the engine with required managers/factories.
    PhotoEngine::PhotoEngine(std::shared_ptr<SourceManager> source_manager,
                             std::shared_ptr<OperationFactory> operation_factory)
        : m_source_manager(source_manager), m_operation_factory(operation_factory) {}

    // Loads a photo file using the SourceManager.
    bool PhotoEngine::loadImage(std::string_view path) {
        // Check if the source manager is valid before attempting to load.
        if (!m_source_manager) {
            // Error handling: SourceManager is required.
            return false;
        }
        // Delegate the loading task to the SourceManager.
        return m_source_manager->loadFile(path);
    }

    // Creates a processing task (Implementation of IProcessingBackend interface).
    std::shared_ptr<IProcessingTask> PhotoEngine::createTask(
        const std::vector<OperationDescriptor>& ops, // List of operations to apply.
        int x, int y, int width, int height          // Region of interest (ROI) for the task.
    ) {
        // Check if required managers are valid.
        if (!m_source_manager || !m_operation_factory) {
            // Error handling: SourceManager and OperationFactory are required.
            return nullptr;
        }

        // 1. Retrieve the image tile (region) from the SourceManager.
        auto tile_unique_ptr = m_source_manager->getTile(x, y, width, height);
        if (!tile_unique_ptr) {
            // Error handling: Failed to retrieve the requested tile.
            return nullptr;
        }

        // 2. Converts the unique_ptr to shared_ptr BEFORE calling it at make_shared
        // We use the shared_ptr constructor which takes a unique_ptr.
    std::shared_ptr<ImageRegion> tile_shared_ptr = std::move(tile_unique_ptr); // Transfère la propriété

        // 3. Creates and returns a new PhotoTask instance with the shared_ptr
        // The order of arguments in the PhotoTask constructor is:
        // (input_tile, ops, operation_factory)
    return std::make_shared<PhotoTask>(tile_shared_ptr, ops, m_operation_factory);
    }

    // Submits a task for execution (Implementation of IProcessingBackend interface).
    bool PhotoEngine::submit(std::shared_ptr<IProcessingTask> task) {
        // Check if the task pointer is valid.
        if (!task) {
            // Error handling: The task pointer is null.
            return false;
        }

        // Execute the task synchronously (Version 1 implementation).
        task->execute();

        // Assume success if execution completes without throwing an exception
        // and the task itself doesn't signal an internal error.
        return true;
    }

    // Commits the result of a completed task back to the source image.
    bool PhotoEngine::commitResult(const std::shared_ptr<IProcessingTask>& task) {
        // Check if the task pointer is valid.
        if (!task) {
            // Error handling: The task pointer is null.
            return false;
        }

        // Check if the source manager is valid before attempting to commit.
        if (!m_source_manager) {
            // Error handling: SourceManager is required.
            return false;
        }

        // Retrieve the processed result from the task.
        auto result = task->result();
        if (!result) {
            // Error handling: The task did not produce a valid result.
            return false;
        }

        // Apply the processed ImageRegion back to the SourceManager.
        return m_source_manager->setTile(*result);
    }

    // Returns the width of the currently loaded image.
    int PhotoEngine::width() const noexcept {
        // Check if the source manager is valid to prevent crashes.
        if (m_source_manager) {
            return m_source_manager->width();
        }
        // Return 0 if no image is loaded or the manager is null.
        return 0;
    }

    // Returns the height of the currently loaded image.
    int PhotoEngine::height() const noexcept {
        if (m_source_manager) {
            return m_source_manager->height();
        }
        return 0;
    }

    // Returns the number of channels of the currently loaded image.
    int PhotoEngine::channels() const noexcept {
        if (m_source_manager) {
            return m_source_manager->channels();
        }
        return 0;
    }

} // namespace CaptureMoment
