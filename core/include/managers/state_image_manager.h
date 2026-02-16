/**
 * @file state_image_manager.h
 * @brief Declaration of StateImageManager class
 *
 * @details
 * This class manages the asynchronous state of the image being edited. It orchestrates
 * the application of active operations to the original image.
 *
 * **Key Architectural Features:**
 * - **Pipeline Context:** Delegates all pipeline construction and strategy management
 *   to the `PipelineContext`. This class focuses solely on application state (operations list,
 *   file paths, threading).
 * - **Asynchronous Updates:** Heavy processing occurs on a worker thread.
 * - **Thread-Safe State Access:** Uses `std::mutex` to protect access to `m_working_image` and other shared state.
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once


#include "operations/operation_descriptor.h"
#include "managers/source_manager.h"
#include "image_processing/interfaces/i_working_image_hardware.h"
#include "operations/operation_factory.h"

#include <vector>
#include <memory>
#include <mutex>
#include <functional>
#include <future>
#include <optional>
#include <string_view>
#include <string>

#include <spdlog/spdlog.h>

namespace CaptureMoment::Core {

namespace Pipeline {
    class PipelineContext; // Necessary also to break tool compilation otherwise we will have error halide.h problems
}

namespace Managers {

/**
 * @class StateImageManager
 * @brief Manages the working image state by applying a sequence of active operations.
 *
 * This class acts as a bridge between the high-level operation list and the low-level
 * execution engine.
 */
class StateImageManager {
public:
    /**
     * @brief Callback type for reporting update completion.
     * @details Uses standard `std::function` for portability across compilers.
     *
     * @param success True if the update and processing were successful, false otherwise.
     */
    using UpdateCallback = std::function<void(bool success)>;

    /**
     * @brief Constructs a StateImageManager.
     *
     * @param source_manager Shared pointer to the SourceManager for accessing original image data.
     * @throws std::invalid_argument if source_manager is null.
     */
    explicit StateImageManager(
        std::shared_ptr<Managers::SourceManager> source_manager);

    /**
     * @brief Destructor.
     */
    ~StateImageManager();

    // Disable copy and assignment
    StateImageManager(const StateImageManager&) = delete;
    StateImageManager& operator=(const StateImageManager&) = delete;

    /**
     * @brief Sets the original image source path.
     *
     * Uses `std::string_view` to accept temporary strings or string literals without allocation.
     * Internally, the path is copied to `std::string` for storage and async use.
     *
     * @param path Path of the loaded image file.
     * @return true if the source was set successfully and image is valid.
     */
    [[nodiscard]] bool setOriginalImageSource(std::string_view path);

    /**
     * @brief Clears all active operations.
     *
     * @return true if reset was initiated successfully.
     */
    [[nodiscard]] bool resetToOriginal();

    /**
     * @brief Requests an asynchronous update of the working image.
     *
     * @param callback Optional callback executed on the worker thread upon completion.
     * @return `std::future<bool>` representing the asynchronous task.
     */
    [[nodiscard]] std::future<bool> requestUpdate(std::optional<UpdateCallback> callback = std::nullopt);

    /**
     * @brief Gets the current working image (Thread-Safe).
     *
     * @return Shared pointer to the current `IWorkingImageHardware`. Returns nullptr if no image exists.
     */
    [[nodiscard]] std::shared_ptr<ImageProcessing::IWorkingImageHardware> getWorkingImage() const;

    /**
     * @brief Checks if a processing update is currently in progress.
     *
     * @return true if the worker thread is processing, false otherwise.
     */
    [[nodiscard]] bool isUpdatePending() const;

    /**
     * @brief Gets the path of the original image source.
     *
     * @return A copy of the file path as `std::string`.
     */
    [[nodiscard]] std::string getOriginalImageSourcePath() const;

    /**
     * @brief Gets a snapshot of the current active operations.
     *
     * @return A copy of the vector of `OperationDescriptor`s.
     */
    [[nodiscard]] std::vector<Operations::OperationDescriptor> getActiveOperations() const;

    /**
     * @brief One atomic list modify operation.
     *
     * @param operations The new list of operations.
     */
    void setActiveOperations(const std::vector<Operations::OperationDescriptor>& operations);

private:
    /**
     * @brief Mutex protecting access to `m_active_operations`, `m_original_image_path`, and `m_working_image`.
     */
    mutable std::mutex m_state_mutex;

    /**
     * @brief The ordered list of operations to apply.
     */
    std::vector<Operations::OperationDescriptor> m_active_operations;

    /**
     * @brief The Pipeline Context infrastructure.
     * @details
     * Owns the Builder and the Halide Strategy. Handles the heavy lifting of pipeline creation.
     */
    std::unique_ptr<Pipeline::PipelineContext> m_pipeline_context;

    /**
     * @brief The current processed image buffer.
     * Protected by `m_state_mutex` for thread-safe access.
     */
    std::shared_ptr<ImageProcessing::IWorkingImageHardware> m_working_image;

    /**
     * @brief File path of the original source image.
     * Kept as `std::string` because ownership is required for async lambda captures.
     */
    std::string m_original_image_path;

    /**
     * @brief Mutex protecting the atomic flag `m_is_updating`.
     */
    mutable std::mutex m_flag_mutex;

    /**
     * @brief Flag preventing multiple concurrent update requests.
     * Protected by `m_flag_mutex` for thread-safe access.
     */
    bool m_is_updating{false};

    /**
     * @brief Factory for creating concrete operation instances.
     */
    std::shared_ptr<Operations::OperationFactory> m_operation_factory;

    /**
     * @brief Dependency to access original image tiles.
     */
    std::shared_ptr<Managers::SourceManager> m_source_manager;

    /**
     * @brief Performs the actual image processing on the worker thread.
     *
     * @param ops_to_apply Copy of the operations list (captured by move).
     * @param original_path Copy of the image path (captured by move).
     * @param callback Optional callback to execute on completion.
     * @return true if processing succeeded, false otherwise.
     */
    [[nodiscard]] bool performUpdate(
        std::vector<Operations::OperationDescriptor> ops_to_apply,
        std::string original_path,
        std::optional<UpdateCallback> callback
        );
};

} // namespace Managers

} // namespace CaptureMoment::Core
