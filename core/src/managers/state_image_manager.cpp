/**
 * @file state_image_manager.cpp
 * @brief Implementation of StateImageManager
 * @author CaptureMoment Team
 * @date 2026
 */

#include "managers/state_image_manager.h"
#include "pipeline/operation_pipeline_builder.h"
#include "managers/source_manager.h"
#include "pipeline/interfaces/i_pipeline_executor.h"

#include "image_processing/factories/working_image_factory.h"
#include "operations/operation_registry.h"

#include "config/app_config.h"

#include <spdlog/spdlog.h>
#include <thread>
#include <future>
#include <format>
#include <utility>
#include <atomic>

namespace CaptureMoment::Core::Managers {

// ============================================================
// Construction / Destruction
// ============================================================

StateImageManager::StateImageManager(
    std::shared_ptr<Managers::SourceManager> source_manager)
    : m_pipeline_builder(std::make_shared<Pipeline::OperationPipelineBuilder>())
    , m_operation_factory(std::make_shared<Operations::OperationFactory>())
    , m_source_manager(std::move(source_manager))
{
    if (!m_source_manager) {
        spdlog::critical("StateImageManager: Null dependency provided during construction.");
        throw std::invalid_argument("StateImageManager: Null dependency provided.");
    }

    Core::Operations::OperationRegistry::registerAll(*m_operation_factory);

    spdlog::debug("StateImageManager: Constructed with fused pipeline support.");
}

StateImageManager::~StateImageManager()
{
    spdlog::debug("StateImageManager: Destroyed.");
}

// ============================================================
// State Management
// ============================================================

bool StateImageManager::setOriginalImageSource(std::string_view path)
{
    std::lock_guard lock(m_mutex);

    if (!m_source_manager || !m_source_manager->isLoaded() || m_source_manager->width() <= 0) {
        spdlog::error("StateImageManager::setOriginalImageSource: SourceManager has no valid image loaded.");
        return false;
    }

    // Copy string_view to std::string member for long-term storage
    m_original_image_path = path;

    spdlog::info("StateImageManager::setOriginalImageSource: Original image source set for '{}'.", m_original_image_path);

    // Reset image pointer to null until first update is requested
    std::atomic_store(&m_working_image, std::shared_ptr<ImageProcessing::IWorkingImageHardware>(nullptr));

    return true;
}

bool StateImageManager::addOperation(const Operations::OperationDescriptor& descriptor)
{
    std::lock_guard lock(m_mutex);
    m_active_operations.push_back(descriptor);
    spdlog::debug("StateImageManager::addOperation: Added operation '{}'. Total active: {}.",
                  descriptor.name, m_active_operations.size());
    return true;
}

bool StateImageManager::modifyOperation(size_t index, const Operations::OperationDescriptor& new_descriptor)
{
    std::lock_guard lock(m_mutex);
    if (index >= m_active_operations.size()) {
        spdlog::error("StateImageManager::modifyOperation: Index {} out of bounds (size: {}).",
                      index, m_active_operations.size());
        return false;
    }
    m_active_operations[index] = new_descriptor;
    spdlog::debug("StateImageManager::modifyOperation: Modified operation at index {}.", index);
    return true;
}

bool StateImageManager::removeOperation(size_t index)
{
    std::lock_guard lock(m_mutex);
    if (index >= m_active_operations.size()) {
        spdlog::error("StateImageManager::removeOperation: Index {} out of bounds (size: {}).",
                      index, m_active_operations.size());
        return false;
    }
    m_active_operations.erase(m_active_operations.begin() + index);
    spdlog::debug("StateImageManager::removeOperation: Removed operation at index {}.", index);
    return true;
}

bool StateImageManager::resetToOriginal()
{
    std::lock_guard lock(m_mutex);
    m_active_operations.clear();
    spdlog::debug("StateImageManager::resetToOriginal: Cleared all operations.");
    return true;
}

// ============================================================
// Accessors
// ============================================================

std::shared_ptr<ImageProcessing::IWorkingImageHardware> StateImageManager::getWorkingImage() const
{
    return m_working_image.load(std::memory_order_acquire);
}

bool StateImageManager::isUpdatePending() const
{
    return m_is_updating.load(std::memory_order_relaxed);
}

std::string StateImageManager::getOriginalImageSourcePath() const
{
    std::lock_guard lock(m_mutex);
    return m_original_image_path;
}

std::vector<Operations::OperationDescriptor> StateImageManager::getActiveOperations() const
{
    std::lock_guard lock(m_mutex);
    return m_active_operations;
}

// ============================================================
// Async Processing Logic
// ============================================================

std::future<bool> StateImageManager::requestUpdate(std::optional<UpdateCallback> callback)
{
    // Atomic exchange: sets to true, returns previous value.
    if (m_is_updating.exchange(true, std::memory_order_acquire))
    {
        spdlog::warn("StateImageManager::requestUpdate: Update already in progress, request ignored.");

        if (callback) {
            callback.value()(false);
        }
        return std::async(std::launch::deferred, []() { return false; });
    }

    spdlog::debug("StateImageManager::requestUpdate: Initiating async update.");

    // Snapshot the state. We move them out for the lambda.
    std::vector<Operations::OperationDescriptor> ops_to_apply;
    std::string original_path;
    {
        std::lock_guard lock(m_mutex);
        ops_to_apply = m_active_operations; // Copy vector
        original_path = m_original_image_path;  // Copy string
    }

    // Launch async task
    auto future = std::async(std::launch::async,
                             [this,
                              ops_to_apply = std::move(ops_to_apply),
                              original_path = std::move(original_path),
                              callback = std::move(callback)]() mutable
                             {
                                 return this->performUpdate(std::move(ops_to_apply), std::move(original_path), std::move(callback));
                             });

    return future;
}

bool StateImageManager::performUpdate(
    std::vector<Operations::OperationDescriptor> ops_to_apply,
    std::string original_path,
    std::optional<UpdateCallback> callback)
{
    std::string thread_id = std::format("0x{:x}", std::hash<std::thread::id>{}(std::this_thread::get_id()));
    spdlog::debug("StateImageManager::performUpdate: Started on thread {}.", thread_id);

    bool success = false;

    spdlog::trace("StateImageManager::performUpdate: Using original path: '{}'", original_path);

    // 1. Retrieve Original Tile from SourceManager
    auto original_tile = m_source_manager->getTile(0, 0, m_source_manager->width(), m_source_manager->height());

    if (!original_tile)
    {
        spdlog::error("StateImageManager::performUpdate (thread {}): Failed to get original tile.", thread_id);
        success = false;
    }
    else
    {
        // 2. Create Working Image (CPU or GPU)
        auto backend = CaptureMoment::Core::Config::AppConfig::instance().getProcessingBackend();
        spdlog::info("StateImageManager::performUpdate: Using backend: {}",
                      (backend == Common::MemoryType::CPU_RAM) ? "CPU" : "GPU");

        // Dereference unique_ptr before passing
        auto unique_new_image = ImageProcessing::WorkingImageFactory::create(backend, *original_tile.value());

        if (!unique_new_image) {
            spdlog::error("StateImageManager::performUpdate (thread {}): WorkingImageFactory::create failed.", thread_id);
            success = false;
        } else
        {
            // 3. Build Fused Pipeline
            auto pipeline_executor = m_pipeline_builder->build(std::move(ops_to_apply), *m_operation_factory);

            if (!pipeline_executor) {
                spdlog::error("StateImageManager::performUpdate (thread {}): OperationPipelineBuilder::build failed.", thread_id);
                success = false;
            } else {
                // 4. Execute Pipeline
                if (!pipeline_executor->execute(*unique_new_image)) {
                    spdlog::error("StateImageManager::performUpdate (thread {}): IPipelineExecutor::execute failed.", thread_id);
                    success = false;
                } else {
                    spdlog::info("StateImageManager::performUpdate (thread {}): Fused pipeline executed successfully on {} operations.",
                                  thread_id, ops_to_apply.size());
                    success = true;

                    // 5. Atomic Swap (Wrapper Method)
                    std::shared_ptr<ImageProcessing::IWorkingImageHardware> shared_new_image(std::move(unique_new_image));
                    m_working_image.store(shared_new_image, std::memory_order_release);

                    spdlog::info("StateImageManager::performUpdate (thread {}): Working image updated successfully.", thread_id);
                }
            }
        }
    }

    // Reset the flag allowing new updates
    m_is_updating.store(false, std::memory_order_release);

    if (callback) {
        callback.value()(success);
    }

    spdlog::debug("StateImageManager::performUpdate: Finished on thread {}.", thread_id);
    return success;
}

} // namespace CaptureMoment::Core::Managers
