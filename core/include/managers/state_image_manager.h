/**
 * @file state_image_manager.h
 * @brief Declaration of StateImageManager class
 *
 * @details
 * This class manages the asynchronous state of the image being edited. It orchestrates
 * the application of active operations to the original image using a fused Halide pipeline.
 *
 * **Key Architectural Features:**
 * - **Fused Pipeline Execution:** Uses `OperationPipelineBuilder` to construct a single
 *   optimized computation graph for all active operations.
 * - **Asynchronous Updates:** Heavy processing occurs on a worker thread via `std::async(std::launch::async)`.
 * - **Lock-Free Double Buffering (C++23):** Implements a lock-free read mechanism for the
 *   `m_working_image` using `std::atomic_load/store`.
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "operations/operation_descriptor.h"
#include "pipeline/operation_pipeline_builder.h"
#include "managers/source_manager.h"
#include "image_processing/interfaces/i_working_image_hardware.h"
#include "operations/operation_factory.h"

#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <functional>
#include <future>
#include <optional>
#include <string_view>
#include <string>

#include <spdlog/spdlog.h>

namespace CaptureMoment::Core {

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
     * @details Uses C++23 `std::move_only_function` for efficient move-only captures.
     *
     * @param success True if the update and processing were successful, false otherwise.
     */
    using UpdateCallback = std::move_only_function<void(bool success)>;

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
     * @brief Adds a new operation to the active sequence.
     *
     * @param descriptor The operation descriptor (parameters, type, etc.).
     * @return true if added successfully.
     */
    [[nodiscard]] bool addOperation(const Operations::OperationDescriptor& descriptor);

    /**
     * @brief Modifies an existing operation in the active sequence.
     *
     * @param index Index of the operation to modify.
     * @param new_descriptor The new operation descriptor.
     * @return true if modified successfully, false if index is invalid.
     */
    [[nodiscard]] bool modifyOperation(size_t index, const Operations::OperationDescriptor& new_descriptor);

    /**
     * @brief Removes an operation from the active sequence.
     *
     * @param index Index of the operation to remove.
     * @return true if removed successfully, false if index is invalid.
     */
    [[nodiscard]] bool removeOperation(size_t index);

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
     * @brief Gets the current working image (Lock-Free).
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

private:
    /**
     * @brief Mutex protecting access to `m_active_operations` and `m_original_image_path`.
     * Note: `m_working_image` is protected by atomic operations.
     */
    mutable std::mutex m_mutex;

    /**
     * @brief The ordered list of operations to apply.
     */
    std::vector<Operations::OperationDescriptor> m_active_operations;

    /**
     * @brief The builder responsible for constructing the fused Halide pipeline.
     */
    std::shared_ptr<Pipeline::OperationPipelineBuilder> m_pipeline_builder;

    /**
     * @brief The current processed image buffer. Atomic Implementation
     * Uses `std::atomic<std::shared_ptr<IWorkingImageHardware>>` to provide
     * lock-free atomic access (load/store) for thread-safe reading/writing
     * of the working image pointer from multiple threads (e.g., UI thread calling
     * `getWorkingImage()` while the worker thread updates `m_working_image`).
     */
    std::atomic<std::shared_ptr<ImageProcessing::IWorkingImageHardware>> m_working_image;

    /**
     * @brief File path of the original source image.
     * Kept as `std::string` because ownership is required for async lambda captures.
     */
    std::string m_original_image_path;

    /**
     * @brief Atomic flag preventing multiple concurrent update requests.
     */
    std::atomic<bool> m_is_updating{false};

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
