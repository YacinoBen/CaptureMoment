/**
 * @file state_image_manager.cpp
 * @brief Implementation of StateImageManager
 * @author CaptureMoment Team
 * @date 2026
 */

#include "managers/state_image_manager.h"

#include "image_processing/factories/working_image_factory.h"
#include "operations/operation_registry.h"
#include "pipeline/pipeline_context.h"
#include "workers/worker_context.h"

#include <spdlog/spdlog.h>
#include <thread>
#include <format>
#include <utility>

namespace CaptureMoment::Core::Managers {

StateImageManager::StateImageManager()
    : m_pipeline_context(std::make_unique<Pipeline::PipelineContext>())
    , m_worker_context(std::make_unique<Workers::WorkerContext>())
    , m_source_manager(std::make_unique<Managers::SourceManager>())
{
    if (!m_source_manager) {
        spdlog::critical("StateImageManager: Null dependency provided during construction.");
        throw std::invalid_argument("StateImageManager: Null dependency provided.");
    }
}

StateImageManager::~StateImageManager()
{
    spdlog::debug("StateImageManager: Destroyed.");
}

// ============================================================
// State Management
// ============================================================

bool StateImageManager::loadImage(std::string_view path)
{
    // 1. Load the file into the INTERNAL SourceManager
    auto load_result = m_source_manager->loadFile(path);

    if (!load_result) {
        spdlog::error("StateImageManager::loadImage: Failed to load file '{}': {}", path, static_cast<int>(load_result.error()));
        return false;
    }

    // 2. Update State Metadata
    {
        std::lock_guard lock(m_state_mutex);
        m_original_image_path = std::string(path);
        // Reset working image pointer
        m_working_image = nullptr;
    }

    spdlog::info("StateImageManager::loadImage: Image '{}' loaded successfully ({}x{}).",
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
        spdlog::error("StateImageManager::commitWorkingImageToSource: No working image available.");
        return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
    }

    // 2. Export to CPU memory
    auto cpu_copy_result = working_image_hw->exportToCPUCopy();
    if (!cpu_copy_result) {
        spdlog::error("StateImageManager::commitWorkingImageToSource: CPU export failed: {}",
                       ErrorHandling::to_string(cpu_copy_result.error()));
        return std::unexpected(cpu_copy_result.error());
    }

    std::unique_ptr<Common::ImageRegion> cpu_copy = std::move(cpu_copy_result.value());

    // 3. Write back to the INTERNAL SourceManager
    if (!m_source_manager->setTile(*cpu_copy)) {
        spdlog::error("StateImageManager::commitWorkingImageToSource: Write to source failed.");
        return std::unexpected(ErrorHandling::CoreError::IOError);
    }

    spdlog::info("StateImageManager: Changes committed to source.");
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
        spdlog::error("StateImageManager::resetToOriginal: Processing failed.");
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
    spdlog::info("StateImageManager::applyOperations: Received {} operations (Move semantics).", ops.size());

    // Check if a previous update is still running
    {
        std::lock_guard lock(m_flag_mutex);
        if (m_is_updating) {
            spdlog::warn("StateImageManager::applyOperations: Update already in progress, request ignored.");
            return std::async(std::launch::deferred, []() { return false; });
        }
        m_is_updating = true;
    }

    // 1. Retrieve the original image data from the SourceManager.
    auto original_tile = m_source_manager->getTile(0, 0, m_source_manager->width(), m_source_manager->height());

    if (!original_tile)
    {
        spdlog::error("StateImageManager::applyOperations: Failed to retrieve original tile.");
        
        // Reset flag since we are returning early
        {
            std::lock_guard lock(m_flag_mutex);
            m_is_updating = false;
        }
        return  std::async(std::launch::deferred, []() { return false; });;
    }

    // 2. Create the Working Image (The execution target).
    auto unique_new_image = ImageProcessing::WorkingImageFactory::create(*original_tile.value());

    if (!unique_new_image) {
        spdlog::error("StateImageManager::applyOperations: Failed to create working image.");
        
        {
            std::lock_guard lock(m_flag_mutex);
            m_is_updating = false;
        }
        return std::async(std::launch::deferred, []() { return false; });
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

    // 7. Handle the asynchronous result (Swap image + Reset flag)
    // We use std::async with deferred launch to create a continuation that will run after the worker's future is ready.
    return std::async(std::launch::deferred, [this, worker_future = std::move(worker_future), working_image = std::move(unique_new_image)]() mutable {
        bool success = worker_future.get();

        if (success) {
            {
                std::lock_guard lock(m_state_mutex);
                m_working_image = std::move(working_image);
                spdlog::info("StateImageManager::applyOperations: Processing completed.");
            }
        } else {
            spdlog::error("StateImageManager::applyOperations: Processing failed.");
        }

        {
            std::lock_guard lock(m_flag_mutex);
            m_is_updating = false;
        }

        return success;
    });
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
