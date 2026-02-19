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
#include <string_view>
#include <string>

#include <spdlog/spdlog.h>

namespace CaptureMoment::Core {

// Forward declarations to avoid circular dependencies
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
 * This class receives processing requests (e.g., apply operations) from the engine,
 * prepares the working memory, delegates execution to workers via the WorkerContext,
 * and updates the final result.
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
     * **Workflow:**
     * 1. Creates a new working image from the source.
     * 2. Initializes the Halide strategy from the context, transferring the operation data.
     * 3. Creates a worker and executes the pipeline asynchronously.
     * 4. Updates the internal working image state upon successful completion.
     *
     * @param ops The list of operation descriptors to apply (moved into the method).
     * @return `std::future<bool>` representing the asynchronous result.
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

    /*
    * @brief Gets the width of the original source image.
    *
    * These methods query the SourceManager for the original image properties.
    */
    [[nodiscard]] int getSourceWidth() const;

    /*
    * @brief Gets the height of the original source image.
    *
    * These methods query the SourceManager for the original image properties.
    */
    [[nodiscard]] int getSourceHeight() const;

    /*
     * @brief Gets the number of channels of the original source image.
     *
     * These methods query the SourceManager for the original image properties.
     */
    [[nodiscard]] int getSourceChannels() const;

private:
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
     * Protected by `m_state_mutex` for thread-safe access.
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
     */
    bool m_is_updating{false};

    /**
     * @brief Factory for creating concrete operation instances.
     */
    std::shared_ptr<Operations::OperationFactory> m_operation_factory;

    /**
     * @brief Dependency to access original image tiles and metadata.
     */
    std::unique_ptr<Managers::SourceManager> m_source_manager;
};

} // namespace Managers

} // namespace CaptureMoment::Core
