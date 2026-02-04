/**
 * @file state_image_manager.h
 * @brief Declaration of StateImageManager class (C++23 Modernized)
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "operations/operation_descriptor.h"
#include "pipeline/operation_pipeline_builder.h"
#include "managers/source_manager.h"
#include "image_processing/interfaces/i_working_image_hardware.h"

#include "operations/operation_factory.h"
#include "pipeline/operation_pipeline_builder.h"

#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <functional>
#include <future>
#include <optional>
#include <string>

#include <spdlog/spdlog.h>

namespace CaptureMoment::Core {

namespace Managers {

/**
 * @brief Manages the working image state by applying a sequence of active operations using fused pipeline execution.
 *
 * This class is responsible for maintaining the current state of the image being edited
 * by applying a list of active operations on the original image using a fused Halide pipeline.
 * It acts as a central point for operation state management, decoupled from UI and history logic.
 * It orchestrates the update of the working image using the OperationPipelineBuilder and OperationPipelineExecutor.
 * The update process is performed asynchronously on a separate thread to avoid blocking the caller.
 *
 * **Thread Safety Model:**
 * - `m_working_image` uses `std::atomic<std::shared_ptr>` for lock-free reads.
 * - `m_active_operations` and `m_original_image_path` are protected by `m_mutex`.
 */
class StateImageManager {
public:
    /**
     * @brief Callback type for reporting update progress or completion.
     * Uses C++23 std::move_only_function to allow efficient transfer of move-only callables.
     * Invoked on the worker thread after the update attempt.
     * @param success True if the update was successful, false otherwise.
     */
    using UpdateCallback = std::move_only_function<void(bool success)>;

    /**
     * @brief Constructs a StateImageManager.
     *
     * Initializes the manager with required dependencies for image processing using fused pipeline execution.
     *
     * @param source_manager Shared pointer to the SourceManager for original image access.
     * @throws std::invalid_argument if the dependency is null.
     */
    explicit StateImageManager(
        std::shared_ptr<Managers::SourceManager> source_manager);

    /**
     * @brief Destructor.
     * Ensures clean shutdown of the manager.
     */
    ~StateImageManager();

    // Disable copy and assignment
    StateImageManager(const StateImageManager&) = delete;
    StateImageManager& operator=(const StateImageManager&) = delete;

    /**
     * @brief Sets the original image source path.
     * This method must be called after the original image is loaded via PhotoEngine
     * to establish the base for all subsequent operations.
     *
     * @param path Path of the loaded image file.
     * @return true if the original image path was successfully set, false otherwise.
     */
    [[nodiscard]] bool setOriginalImageSource(const std::string& path);

    /**
     * @brief Adds a new operation to the active sequence.
     * This operation is appended to the end of the list. The working image update is triggered.
     * The update itself happens asynchronously using fused pipeline execution.
     *
     * @param descriptor The operation descriptor to add.
     * @return true if the operation was added successfully.
     */
    [[nodiscard]] bool addOperation(const Operations::OperationDescriptor& descriptor);

    /**
     * @brief Modifies an existing operation in the active sequence.
     * The working image update is triggered after modification using fused pipeline execution.
     * The update itself happens asynchronously.
     *
     * @param index Index of the operation to modify within the active sequence.
     * @param new_descriptor The new operation descriptor.
     * @return true if the operation was modified successfully, false if index is out of bounds.
     */
    [[nodiscard]] bool modifyOperation(size_t index, const Operations::OperationDescriptor& new_descriptor);

    /**
     * @brief Removes an operation from the active sequence.
     * The working image update is triggered after removal using fused pipeline execution.
     * The update itself happens asynchronously.
     *
     * @param index Index of the operation to remove within the active sequence.
     * @return true if the operation was removed successfully, false if index is out of bounds.
     */
    [[nodiscard]] bool removeOperation(size_t index);

    /**
     * @brief Clears all active operations, resetting the working image to the original state.
     * The working image update is triggered, effectively re-applying an empty operation list using fused pipeline execution.
     * The update itself happens asynchronously.
     *
     * @return true if the reset process was initiated successfully.
     */
    [[nodiscard]] bool resetToOriginal();

    /**
     * @brief Requests an asynchronous update of the working image based on the current sequence of operations.
     * This method initiates the update process on a separate worker thread using std::async.
     * It captures the current state of active operations and the original image path to ensure
     * consistency during the potentially long-running update process.
     * If an update is already in progress, this request is ignored.
     *
     * @param callback Optional callback function to be executed on the worker thread after the update attempt.
     * The callback receives a boolean indicating the success of the update.
     * @return A std::future<bool> that can be used by the caller to wait for the update to complete
     * and retrieve the final success status. The future is ready when the update finishes.
     */
    [[nodiscard]] std::future<bool> requestUpdate(std::optional<UpdateCallback> callback = std::nullopt);

    /**
     * @brief Gets the current working image hardware abstraction.
     * This method is lock-free (uses atomic_load) and provides safe access to the working image.
     * The returned shared_ptr ensures the image remains valid for the caller even if the manager updates its internal state.
     * @return Shared pointer to the current IWorkingImageHardware, or nullptr if not loaded.
     */
    [[nodiscard]] std::shared_ptr<ImageProcessing::IWorkingImageHardware> getWorkingImage() const;

    /**
     * @brief Checks if an update of the working image is currently in progress.
     * This method provides a thread-safe way to determine if the internal state
     * (specifically m_working_image) might be changing due to an active update task.
     *
     * @return true if an update is ongoing, false otherwise.
     */
    [[nodiscard]] bool isUpdatePending() const;

    /**
     * @brief Gets the file path of the original image source.
     * This path was set via setOriginalImageSource.
     *
     * @return The file path of the original image, or an empty string if not set.
     */
    [[nodiscard]] std::string getOriginalImageSourcePath() const;

    /**
     * @brief Gets the current list of active operations.
     * This method provides a thread-safe snapshot of the operations
     * that are currently applied to generate the working image.
     *
     * @return A copy of the vector containing the active OperationDescriptors.
     */
    [[nodiscard]] std::vector<Operations::OperationDescriptor> getActiveOperations() const;

private:
    /**
     * @brief Mutex protecting concurrent access to m_active_operations and m_original_image_path.
     * m_working_image is protected by its own atomic nature and does not require this mutex.
     */
    mutable std::mutex m_mutex;
    /**
     * @brief The ordered list of operation descriptors to apply to the original image.
     * This vector represents the current sequence of active operations that define the state of the working image.
     */
    std::vector<Operations::OperationDescriptor> m_active_operations;

    /**
     * @brief Shared pointer to the pipeline builder responsible for creating fused operation pipelines.
     */
    std::shared_ptr<Pipeline::OperationPipelineBuilder> m_pipeline_builder;

    /**
     * @brief The current state of the processed image.
     * Uses std::atomic<std::shared_ptr> for lock-free thread-safe reads.
     * This allows the UI thread to access the image without blocking on a mutex,
     * even while a background update is preparing the next image.
     */
    std::atomic<std::shared_ptr<ImageProcessing::IWorkingImageHardware>> m_working_image;

    /**
     * @brief The file path of the original image loaded into the system.
     * This path is used as the base for all operations and to retrieve the original image data.
     */
    std::string m_original_image_path;
    /**
     * @brief Atomic flag indicating whether an asynchronous update task is currently running.
     * This flag is used to prevent multiple simultaneous update requests.
     */
    std::atomic<bool> m_is_updating {false};

    /**
     * @brief Shared pointer to the factory responsible for creating operation instances.
     */
    std::shared_ptr<Operations::OperationFactory> m_operation_factory;

    /**
     * @brief Shared pointer to the SourceManager dependency.
     * Used to retrieve the original image data from the source file.
     */
    std::shared_ptr<Managers::SourceManager> m_source_manager;

    /**
     * @brief Performs the core image update logic on a worker thread using fused pipeline execution.
     * This private method is executed by std::async. It retrieves the original image,
     * builds a fused pipeline using the pipeline builder, executes it on the working image,
     * and updates the internal m_working_image member in a thread-safe manner.
     * It also handles the execution of the optional callback.
     *
     * @param ops_to_apply The sequence of operations to apply to the original image.
     * @param original_path The path to the original image file (used for retrieval).
     * @param callback Optional callback to execute after the update attempt.
     * @return true if the update process (retrieval, processing, internal update) was successful, false otherwise.
     */
    [[nodiscard]] bool performUpdate(
        const std::vector<Operations::OperationDescriptor>& ops_to_apply,
        const std::string& original_path,
        const std::optional<UpdateCallback> callback
        );
};

} // namespace Managers

} // namespace CaptureMoment::Core
