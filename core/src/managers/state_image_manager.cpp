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

#include <spdlog/spdlog.h>
#include <thread>
#include <format>
#include <utility>

namespace CaptureMoment::Core::Managers {

StateImageManager::StateImageManager(
    std::shared_ptr<Managers::SourceManager> source_manager)
    : m_pipeline_context(std::make_unique<Pipeline::PipelineContext>()) 
    , m_operation_factory(std::make_shared<Operations::OperationFactory>())
    , m_source_manager(std::move(source_manager))
{
    if (!m_source_manager) {
        spdlog::critical("StateImageManager: Null dependency provided during construction.");
        throw std::invalid_argument("StateImageManager: Null dependency provided.");
    }

    Core::Operations::OperationRegistry::registerAll(*m_operation_factory);
    spdlog::debug("StateImageManager: Constructed with persistent pipeline executor support.");
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
    std::lock_guard lock(m_state_mutex);

    if (!m_source_manager || !m_source_manager->isLoaded() || m_source_manager->width() <= 0) {
        spdlog::error("StateImageManager::setOriginalImageSource: SourceManager has no valid image loaded.");
        return false;
    }

    // Copy string_view to std::string member for long-term storage
    m_original_image_path = path;

    spdlog::info("StateImageManager::setOriginalImageSource: Original image source set for '{}'.", m_original_image_path);

    // Reset image pointer to null until first update is requested
    // Using m_state_mutex to protect m_working_image now
    m_working_image = nullptr;

    return true;
}

bool StateImageManager::addOperation(const Operations::OperationDescriptor& descriptor)
{
    std::lock_guard lock(m_state_mutex);
    m_active_operations.push_back(descriptor);
    spdlog::debug("StateImageManager::addOperation: Added operation '{}'. Total active: {}.",
                  descriptor.name, m_active_operations.size());
    return true;
}

bool StateImageManager::modifyOperation(size_t index, const Operations::OperationDescriptor& new_descriptor)
{
    std::lock_guard lock(m_state_mutex);
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
    std::lock_guard lock(m_state_mutex);
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
    std::lock_guard lock(m_state_mutex);
    m_active_operations.clear();
    spdlog::debug("StateImageManager::resetToOriginal: Cleared all operations.");
    return true;
}

std::shared_ptr<ImageProcessing::IWorkingImageHardware> StateImageManager::getWorkingImage() const
{
    std::lock_guard lock(m_state_mutex); // Protect access to m_working_image
    return m_working_image;
}

bool StateImageManager::isUpdatePending() const
{
    std::lock_guard lock(m_flag_mutex); // Protect access to m_is_updating
    return m_is_updating;
}

std::string StateImageManager::getOriginalImageSourcePath() const
{
    std::lock_guard lock(m_state_mutex);
    return m_original_image_path;
}

std::vector<Operations::OperationDescriptor> StateImageManager::getActiveOperations() const
{
    std::lock_guard lock(m_state_mutex);
    return m_active_operations;
}

std::future<bool> StateImageManager::requestUpdate(std::optional<UpdateCallback> callback)
{
    {
        std::lock_guard lock(m_flag_mutex);
        if (m_is_updating)
        {
            spdlog::warn("StateImageManager::requestUpdate: Update already in progress, request ignored.");
            if (callback) {
                callback.value()(false);
            }
            return std::async(std::launch::deferred, []() { return false; });
        }
        m_is_updating = true;
    }

    spdlog::debug("StateImageManager::requestUpdate: Initiating async update.");

    std::vector<Operations::OperationDescriptor> ops_to_apply;
    std::string original_path;
    {
        std::lock_guard lock(m_state_mutex);
        ops_to_apply = m_active_operations;
        original_path = m_original_image_path;
    }

    // Launch async task
    auto future = std::async(std::launch::async,
                             [this,
                              ops_to_apply = std::move(ops_to_apply),
                              original_path = std::move(original_path),
                              callback = std::move(callback)]() mutable
                             {
                                 bool result = this->performUpdate(std::move(ops_to_apply), std::move(original_path), std::move(callback));

                                 {
                                     std::lock_guard lock(this->m_flag_mutex);
                                     this->m_is_updating = false;
                                 }

                                 return result;
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
        auto unique_new_image = ImageProcessing::WorkingImageFactory::create(*original_tile.value());

        if (!unique_new_image) {
            spdlog::error("StateImageManager::performUpdate (thread {}): WorkingImageFactory::create failed.", thread_id);
            success = false;
        } else
        {
            // 3. INITIALIZATION & EXECUTION via PipelineContext
            
            auto& halide_operations_manager = m_pipeline_context->getHalideManager();

            // 3a. Initialiser la stratégie (Ceci met à jour l'exécuteur interne avec les nouvelles opérations)
            spdlog::trace("StateImageManager::performUpdate: Initializing Halide Strategy with {} ops.", ops_to_apply.size());
            halide_operations_manager.init(ops_to_apply, *m_operation_factory);

            // 3b. Exécuter la stratégie
            if (halide_operations_manager.execute(*unique_new_image)) {
                spdlog::info("StateImageManager::performUpdate (thread {}): Fused pipeline executed successfully on {} operations.",
                             thread_id, ops_to_apply.size());
                success = true;

                // 4. Update Working Image (Thread-Safe)
                {
                    std::lock_guard lock(m_state_mutex);
                    m_working_image = std::move(unique_new_image);
                }

                spdlog::info("StateImageManager::performUpdate (thread {}): Working image updated successfully.", thread_id);
            } else {
                spdlog::error("StateImageManager::performUpdate (thread {}): Pipeline execution failed.", thread_id);
                success = false;
            }
        }
    }

    // Callback invocation
    if (callback) {
        callback.value()(success);
    }

    spdlog::debug("StateImageManager::performUpdate: Finished on thread {}.", thread_id);
    return success;
}

} // namespace CaptureMoment::Core::Managers
