/**
 * @file state_image_manager.h
 * @brief Declaration of StateImageManager class
 *
 * @details
 * This class manages the asynchronous processing state of the image being edited.
 * It acts as a coordinator between the image source, the infrastructure contexts
 * (Pipeline & Workers), and the working image memory.
 *
 * **Key Architectural Features:**
 * - **Contexts:** Owns `PipelineContext` (Strategies) and `WorkerContext` (Executors).
 * - **Move Semantics:** All data processing methods accept ownership of data via `std::move`.
 * - **Thread-Safe:** Uses mutexes to protect the working image and status flags.
 * - **Stateless Processing:** Does not store operation lists; it receives, processes, and discards.
 * - **Coalescing Updates:** If an update is requested while another is in progress,
 *   the new request replaces any previously pending request, optimizing for the most
 *   recent state during rapid UI interactions (e.g., dragging a slider).
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "operations/operation_descriptor.h"
#include "managers/i_source_manager.h"
#include "image_processing/interfaces/i_working_image_hardware.h"
#include "operations/operation_factory.h"
#include "common/types/image_types.h"

#include <vector>
#include <memory>
#include <mutex>
#include <future>
#include <string_view>
#include <string>
#include <optional>


namespace CaptureMoment::Core {

namespace Pipeline {
class PipelineContext;
}

namespace Workers {
class WorkerContext;
}

namespace Managers {

/**
 * @class StateImageManager
 * @brief Manages the working image lifecycle and coordinates asynchronous processing.
 *
 * @details
 * This class receives processing requests (e.g., apply operations) from the engine,
 * prepares the working memory, delegates execution to workers via the WorkerContext,
 * and updates the final result. It implements a coalescing strategy for incoming
 * operations to optimize responsiveness during rapid UI interactions.
 *
 * **Coalescing Behavior:**
 * - When `applyOperations` is called and an update is already `isUpdatePending()`,
 *   the new operation list is stored as pending.
 * - If another `applyOperations` call occurs while a *previous* one is pending,
 *   the *newest* operation list **overwrites** the previously pending one.
 * - Once the currently running update finishes, if a pending operation exists,
 *   it is launched immediately, continuing the chain. If no pending operation exists,
 *   the update state is reset.
 * - The `std::future<bool>` returned by `applyOperations` resolves only when the
 *   operation *actually executed* (either the initial one or the final coalesced one)
 *   completes.
 */
class StateImageManager {
public:
    /**
     * @brief Constructs a StateImageManager.
     *
     * @details
     * Initializes the PipelineContext and WorkerContext.
     *
     * @throws std::invalid_argument if source_manager is null.
     */
    explicit StateImageManager();

    /**
     * @brief Destructor.
     */
    ~StateImageManager();

    // Disable copy and assignment
    StateImageManager(const StateImageManager&) = delete;
    StateImageManager& operator=(const StateImageManager&) = delete;

    /**
     * @brief Loads an image file into the internal SourceManager.
     * @param path Path to the image file.
     * @return true if successful.
     */
    [[nodiscard]] bool loadImage(std::string_view path);

    /**
     * @brief Commits the current working image (with applied operations) back to the SourceManager.
     *
     * @details
     * This effectively overwrites the original image data with the processed version.
     * Future resets or reloads will use this new "original" data.
     *
     * @return std::expected with void or error.
     */
    [[nodiscard]] std::expected<void, ErrorHandling::CoreError> commitWorkingImageToSource();

    /**
     * @brief Resets the image to its original state (synchronous).
     *
     * @details
     * This method applies an empty list of operations (reset) and waits for completion.
     * It is used by `PhotoEngine::loadImage` to ensure the image is ready immediately.
     *
     * @return `std::expected<void, CoreError>` indicating success or failure.
     */
    [[nodiscard]] std::expected<void, ErrorHandling::CoreError> resetToOriginal();

    /**
     * @brief Applies a list of operations to the current image using move semantics.
     *
     * @details
     * This method is the primary entry point for image processing. It takes ownership
     * of the provided operation descriptors via `std::move` to avoid unnecessary copies.
     *
     * **Coalescing Workflow:**
     * 1. If `isUpdatePending()` is true, the `ops` are stored as the new pending request
     *    (overwriting any older pending request) and a new `std::future<bool>` is returned.
     *    The processing chain continues when the current operation finishes.
     * 2. If `isUpdatePending()` is false, the `ops` are processed immediately via `launchProcessing`.
     *
     * **Standard Workflow (when no pending update):**
     * 1. Creates a new working image from the source.
     * 2. Initializes the Halide strategy from the context, transferring the operation data.
     * 3. Creates a worker and executes the pipeline asynchronously.
     * 4. Updates the internal working image state upon successful completion.
     *
     * @param ops The list of operation descriptors to apply (moved into the method).
     * @return `std::future<bool>` representing the asynchronous result.
     *         The future resolves when the operation actually executed completes.
     */
    [[nodiscard]] std::future<bool> applyOperations(std::vector<Operations::OperationDescriptor>&& ops);

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
     * @brief Gets the width of the original source image.
     *
     * @details These methods query the SourceManager for the original image properties.
     * @return The width of the source image.
     */
    [[nodiscard]] Common::ImageDim getSourceWidth() const;

    /**
     * @brief Gets the height of the original source image.
     *
     * @details These methods query the SourceManager for the original image properties.
     * @return The height of the source image.
     */
    [[nodiscard]] Common::ImageDim getSourceHeight() const;

    /**
     * @brief Gets the number of channels of the original source image.
     *
     * @details These methods query the SourceManager for the original image properties.
     * @return The number of channels of the source image.
     */
    [[nodiscard]] Common::ImageChan getSourceChannels() const;

private:

    // ========================================================================
    // Internal Processing Methods
    // ========================================================================

    /**
     * @brief Launches async processing pipeline for operations.
     * @param ops Operations to process.
     */
    void launchProcessing(std::vector<Operations::OperationDescriptor> ops);

    /**
     * @brief Handles processing completion and triggers pending ops if exists.
     * @param success Whether processing succeeded.
     */
    void onProcessingComplete(bool success);

    /**
     * @brief Blocks until all pending processing completes.
     */
    void waitForPendingProcessing();


    /**
     * @brief Mutex protecting access to `m_working_image` and `m_original_image_path`.
     */
    mutable std::mutex m_state_mutex;

    /**
     * @brief The Pipeline Context infrastructure.
     * @details
     * Owns the Builder and the Strategy Managers (Halide, Sky, etc.).
     */
    std::unique_ptr<Pipeline::PipelineContext> m_pipeline_context;

    /**
     * @brief The Worker Context infrastructure.
     * @details
     * Owns the logic to create and retrieve processing workers.
     */
    std::unique_ptr<Workers::WorkerContext> m_worker_context;

    /**
     * @brief The current processed image buffer.
     * @details Protected by `m_state_mutex` for thread-safe access.
     */
    std::shared_ptr<ImageProcessing::IWorkingImageHardware> m_working_image;

    /**
     * @brief File path of the original source image.
     */
    std::string m_original_image_path;

    /**
     * @brief Mutex protecting the atomic flag `m_is_updating`.
     */
    mutable std::mutex m_flag_mutex;

    /**
     * @brief Flag preventing multiple concurrent update requests.
     * @details Uses atomic<bool> for lock-free reads in `isUpdatePending()`.
     */
    std::atomic<bool> m_is_updating{false};

    /**
     * @brief Dependency to access original image tiles and metadata.
     */
    std::unique_ptr<Managers::ISourceManager> m_source_manager;


    /**
     * @brief Mutex protecting access to `m_pending_ops` and `m_pending_promise`.
     */
    mutable std::mutex m_pending_mutex;

    /**
     * @brief Stores the most recent operation list requested while an update is in progress.
     * @details If `has_value()`, this operation will be processed next.
     */
    std::optional<std::vector<Operations::OperationDescriptor>> m_pending_ops;

    /**
     * @brief Promise associated with the currently executing or last completed operation chain.
     * @details The future returned by the initial `applyOperations` call in a chain
     *          is resolved by this promise when the chain (potentially including a coalesced
     *          operation) finishes.
     */
    std::promise<bool> m_pending_promise;
};

} // namespace Managers

} // namespace CaptureMoment::Core
