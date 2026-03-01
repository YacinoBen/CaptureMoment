/**
 * @file state_image_manager.cpp
 * @brief Implementation of StateImageManager
 * @author CaptureMoment Team
 * @date 2026
 */

#include "managers/state_image_manager.h"

#include "image_processing/factories/working_image_factory.h"
#include "pipeline/pipeline_context.h"
#include "workers/worker_context.h"
#include "managers/source_manager.h"

#include <spdlog/spdlog.h>
#include <utility>

namespace CaptureMoment::Core::Managers {

StateImageManager::StateImageManager()
    : m_pipeline_context(std::make_unique<Pipeline::PipelineContext>())
    , m_worker_context(std::make_unique<Workers::WorkerContext>())
    , m_source_manager(std::make_unique<Managers::SourceManager>())
{
    if (!m_source_manager) {
        spdlog::critical("[StateImageManager::StateImageManager]: Null dependency provided during construction.");
        throw std::invalid_argument("[StateImageManager::StateImageManager]: Null dependency provided.");
    }
}

StateImageManager::~StateImageManager()
{
    // Wait for any pending processing to complete before destruction
    while (m_is_updating) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    spdlog::debug("[StateImageManager::~StateImageManager]: Destroyed.");
}

// ============================================================
// State Management
// ============================================================

bool StateImageManager::loadImage(std::string_view path)
{
    // 1. Wait for any pending processing to complete
    waitForPendingProcessing();

    // 2. Load the file into the INTERNAL SourceManager
    auto load_result = m_source_manager->loadFile(path);

    if (!load_result) {
        spdlog::error("[StateImageManager::loadImage]: Failed to load file '{}': {}", path, static_cast<int>(load_result.error()));
        return false;
    }

    // 3. Update State Metadata
    {
        std::lock_guard lock(m_state_mutex);
        m_original_image_path = std::string(path);
        // Reset working image pointer
        m_working_image = nullptr;
    }

    // 4. Clear any pending operations from previous image
    {
        std::lock_guard lock(m_pending_mutex);
        m_pending_ops.reset();
    }
    spdlog::info("[StateImageManager::loadImage]: Image '{}' loaded successfully ({}x{}).",
                 path, m_source_manager->width(), m_source_manager->height());

    return true;
}

std::expected<void, ErrorHandling::CoreError> StateImageManager::commitWorkingImageToSource()
{
    // 1. Retrieve the current working image
    // Note: getWorkingImage() handles its own locking, but we might want to lock m_state_mutex
    // if we want to ensure the image doesn't change during this export (snapshot behavior).
    // For now, we assume the caller handles synchronization or accepts the race condition (latest frame).
    std::shared_ptr<ImageProcessing::IWorkingImageHardware> working_image_hw;
    {
        std::lock_guard lock(m_state_mutex);
        working_image_hw = m_working_image;
    }

    if (!working_image_hw) {
        spdlog::error("[StateImageManager::commitWorkingImageToSource]: No working image available.");
        return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
    }

    // 2. Export to CPU memory
    auto cpu_copy_result = working_image_hw->exportToCPUCopy();
    if (!cpu_copy_result) {
        spdlog::error("[StateImageManager::commitWorkingImageToSource]: CPU export failed: {}",
                       ErrorHandling::to_string(cpu_copy_result.error()));
        return std::unexpected(cpu_copy_result.error());
    }

    std::unique_ptr<Common::ImageRegion> cpu_copy = std::move(cpu_copy_result.value());

    // 3. Write back to the INTERNAL SourceManager
    if (!m_source_manager->setTile(*cpu_copy)) {
        spdlog::error("[StateImageManager::commitWorkingImageToSource]: Write to source failed.");
        return std::unexpected(ErrorHandling::CoreError::IOError);
    }

    spdlog::info("[StateImageManager::commitWorkingImageToSource]: Changes committed to source.");
    return {};
}

std::expected<void, ErrorHandling::CoreError> StateImageManager::resetToOriginal()
{
    // 1. Trigger the async processing with an empty list of operations
    auto future = applyOperations(std::vector<Operations::OperationDescriptor>{});

    // 2. Block and wait for the operation to complete
    // This allows the caller (PhotoEngine) to assume the image is ready when this returns.
    bool success = future.get();

    // 3. Convert boolean result to std::expected
    if (!success) {
        spdlog::error("[StateImageManager::resetToOriginal]: Processing failed.");
        return std::unexpected(ErrorHandling::CoreError::AllocationFailed);
    }

    return {};
}

std::shared_ptr<ImageProcessing::IWorkingImageHardware> StateImageManager::getWorkingImage() const
{
    std::lock_guard lock(m_state_mutex);
    return m_working_image;
}

bool StateImageManager::isUpdatePending() const
{
    std::lock_guard lock(m_flag_mutex);
    return m_is_updating;
}

std::string StateImageManager::getOriginalImageSourcePath() const
{
    std::lock_guard lock(m_state_mutex);
    return m_original_image_path;
}

std::future<bool> StateImageManager::applyOperations(std::vector<Operations::OperationDescriptor>&& ops)
{
    spdlog::info("[StateImageManager::applyOperations]: Received {} operations (Move semantics).", ops.size());

    // ============================================================
    // CASE 1: Processing already in progress → COALESCE
    // ============================================================
    if (m_is_updating.load(std::memory_order_acquire))
    {
        std::lock_guard lock(m_pending_mutex);

        // Overwrite any previous pending operations
        m_pending_ops = std::move(ops);

        // Create a new promise for this caller
        // The previous promise will be fulfilled when the current chain completes
        m_pending_promise = std::promise<bool>();

        spdlog::debug("[StateImageManager::applyOperations]: Processing in progress, "
                      "ops stored as pending (coalesced).");

        return m_pending_promise.get_future();
    }

    // ============================================================
    // CASE 2: No processing in progress → LAUNCH DIRECTLY
    // ============================================================

    // Create promise for this request
    m_pending_promise = std::promise<bool>();
    auto future = m_pending_promise.get_future();

    // Launch the processing
    launchProcessing(std::move(ops));

    return future;
}

void StateImageManager::launchProcessing(
    std::vector<Operations::OperationDescriptor> ops)
{
    spdlog::trace("[StateImageManager::launchProcessing]: Starting async processing.");

    // Set the updating flag
    m_is_updating.store(true, std::memory_order_release);

    // 1. Retrieve the original image data from the SourceManager.
    auto original_tile = m_source_manager->getTile(0, 0, m_source_manager->width(), m_source_manager->height());

    if (!original_tile)
    {
        spdlog::error("[StateImageManager::launchProcessing]: Failed to retrieve original tile.");
        onProcessingComplete(false);
        return;
    }

    // 2. Create the Working Image (The execution target).
    auto unique_new_image = ImageProcessing::WorkingImageFactory::create(*original_tile.value());

    if (!unique_new_image) {
        spdlog::error("[StateImageManager::launchProcessing]: Failed to create working image.");
        onProcessingComplete(false);
        return;
    }

    // 3. Retrieve the Halide Manager from the Pipeline Context.
    auto& halide_manager = m_pipeline_context->getHalideManager();

    // 4. Initialize the Manager with the Operations (Move Data Transfer).
    // This transfers ownership of `ops` to the manager.
    halide_manager.init(std::move(ops));

    // 5. Retrieve the specific Worker for Halide operations.
    auto worker = m_worker_context->getHalideOperationWorker();

    // 6. Execute the processing asynchronously.
    // Pass the Context (infrastructure) and the Image (data).
    auto worker_future = worker.execute(*m_pipeline_context, *unique_new_image);

    // 7. Launch async continuation to handle completion
    std::thread([this, worker_future = std::move(worker_future),
                 working_image = std::move(unique_new_image)]() mutable {
        // Wait for worker to complete
        bool success = worker_future.get();

        // Update working image on success
        if (success) {
            std::lock_guard lock(m_state_mutex);
            m_working_image = std::move(working_image);
            spdlog::info("[StateImageManager::launchProcessing]: Processing completed.");
        } else {
            spdlog::error("[StateImageManager::launchProcessing]: Processing failed.");
        }

        // Handle completion (check for pending ops)
        onProcessingComplete(success);
    }).detach();
}

void StateImageManager::onProcessingComplete(bool success)
{
    spdlog::trace("[StateImageManager::onProcessingComplete]: Checking for pending operations.");

    // ============================================================
    // Check for pending operations
    // ============================================================
    std::optional<std::vector<Operations::OperationDescriptor>> next_ops;
    {
        std::lock_guard lock(m_pending_mutex);
        next_ops = std::move(m_pending_ops);
        m_pending_ops.reset();
    }

    // ============================================================
    // CASE A: Pending operations exist → RELAUNCH
    // ============================================================
    if (next_ops.has_value() && !next_ops->empty()) {
        spdlog::debug("[StateImageManager::onProcessingComplete]: "
                      "Launching {} pending operations.", next_ops->size());

        // Keep m_is_updating = true, launch next processing
        launchProcessing(std::move(*next_ops));
        return;
    }

    // ============================================================
    // CASE B: No pending operations → COMPLETE
    // ============================================================

    // Reset the updating flag
    m_is_updating.store(false, std::memory_order_release);

    // Fulfill the promise
    try {
        m_pending_promise.set_value(success);
    } catch (const std::future_error& e) {
        // Promise already satisfied (should not happen, but log just in case)
        spdlog::warn("[StateImageManager::onProcessingComplete]: Promise error: {}", e.what());
    }

    spdlog::debug("[StateImageManager::onProcessingComplete]: All processing complete.");
}

void StateImageManager::waitForPendingProcessing()
{
    while (m_is_updating.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

int StateImageManager::getSourceWidth() const
{
    std::lock_guard lock(m_state_mutex);
    return m_source_manager ? m_source_manager->width() : 0;
}

int StateImageManager::getSourceHeight() const
{
    std::lock_guard lock(m_state_mutex);
    return m_source_manager ? m_source_manager->height() : 0;
}

int StateImageManager::getSourceChannels() const
{
    std::lock_guard lock(m_state_mutex);
    return m_source_manager ? m_source_manager->channels() : 0;
}

} // namespace CaptureMoment::Core::Managers
