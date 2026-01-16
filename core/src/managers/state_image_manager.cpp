/**
 * @file state_image_manager.cpp
 * @brief Implementation of StateImageManager
 * @author CaptureMoment Team
 * @date 2025
 */

#include "managers/state_image_manager.h"
#include "operations/operation_pipeline.h"
#include "managers/source_manager.h"

#include "config/app_config.h"

#include "image_processing/image_processing.h"

#include <spdlog/spdlog.h>
#include <thread>
#include <future>
#include <sstream>

namespace CaptureMoment::Core::Managers {

// Constructs a StateImageManager with required dependencies.
StateImageManager::StateImageManager(
    std::shared_ptr<Managers::SourceManager> source_manager,
    std::shared_ptr<Operations::OperationPipeline> operation_pipeline,
    std::shared_ptr<Operations::OperationFactory> operation_factory
    )
    : m_source_manager(std::move(source_manager))
    , m_operation_pipeline(std::move(operation_pipeline))
    , m_operation_factory(std::move(operation_factory))
{
    if (!m_source_manager || !m_operation_pipeline || !m_operation_factory) {
        spdlog::critical("StateImageManager: Null dependency provided during construction.");
        throw std::invalid_argument("StateImageManager: Null dependency provided.");
    }
    spdlog::debug("StateImageManager: Constructed.");
}

// Destructor for StateImageManager.
StateImageManager::~StateImageManager()
{
    spdlog::debug("StateImageManager: Destroyed.");
}

// Sets the original image source path.
bool StateImageManager::setOriginalImageSource(const std::string& path)
{
    std::lock_guard lock(m_mutex);
    if (!m_source_manager || !m_source_manager->isLoaded() || m_source_manager->width() <= 0) {
        spdlog::error("StateImageManager::setOriginalImageSource: SourceManager has no valid image loaded.");
        return false;
    }
    m_original_image_path = path;
    spdlog::info("StateImageManager::setOriginalImageSource: Original image source set for '{}'.", path);
    return true;
}

// Adds a new operation to the active sequence.
bool StateImageManager::addOperation(const Operations::OperationDescriptor& descriptor)
{
    std::lock_guard lock(m_mutex);
    m_active_operations.push_back(descriptor);
    spdlog::debug("StateImageManager::addOperation: Added operation '{}'. Total active: {}.", descriptor.name, m_active_operations.size());
    return true;
}

// Modifies an existing operation in the active sequence.
bool StateImageManager::modifyOperation(size_t index, const Operations::OperationDescriptor& new_descriptor)
{
    std::lock_guard lock(m_mutex);
    if (index >= m_active_operations.size()) {
        spdlog::error("StateImageManager::modifyOperation: Index {} out of bounds (size: {}).", index, m_active_operations.size());
        return false;
    }
    m_active_operations[index] = new_descriptor;
    spdlog::debug("StateImageManager::modifyOperation: Modified operation at index {}. Total active: {}.", index, m_active_operations.size());
    return true;
}

// Removes an operation from the active sequence.
bool StateImageManager::removeOperation(size_t index)
{
    std::lock_guard lock(m_mutex);
    if (index >= m_active_operations.size()) {
        spdlog::error("StateImageManager::removeOperation: Index {} out of bounds (size: {}).", index, m_active_operations.size());
        return false;
    }
    m_active_operations.erase(m_active_operations.begin() + index);
    spdlog::debug("StateImageManager::removeOperation: Removed operation at index {}. Total active: {}.", index, m_active_operations.size());
    return true;
}

// Clears all active operations, resetting the working image to the original.
bool StateImageManager::resetToOriginal()
{
    std::lock_guard lock(m_mutex);
    m_active_operations.clear();
    spdlog::debug("StateImageManager::resetToOriginal: Cleared all operations.");
    return true;
}

// Requests an asynchronous update of the working image.
std::future<bool> StateImageManager::requestUpdate(std::optional<UpdateCallback> callback)
{
    bool was_updating = m_is_updating.exchange(true);
    if (was_updating)
    {
        spdlog::warn("StateImageManager::requestUpdate: Update already in progress, request ignored.");
        if (callback.has_value()) {
            callback.value()(false);
        }
        return std::async(std::launch::deferred, []() { return false; });
    }

    spdlog::debug("StateImageManager::requestUpdate: Initiating async update.");

    std::vector<Operations::OperationDescriptor> ops_to_apply;
    std::string original_path;
    {
        std::lock_guard lock(m_mutex);
        ops_to_apply = m_active_operations;
        original_path = m_original_image_path;
    }

    auto future = std::async(std::launch::async, [this, ops_to_apply, original_path, callback]() {
        return this->performUpdate(ops_to_apply, original_path, callback);
    });

    return future;
}

// Gets the current working image.
ImageProcessing::IWorkingImageHardware* StateImageManager::getWorkingImage() const
{
    std::lock_guard lock(m_mutex);
    return m_working_image.get();
}

// Checks if an update of the working image is currently in progress.
bool StateImageManager::isUpdatePending() const
{
    return m_is_updating.load();
}

bool StateImageManager::performUpdate(
    const std::vector<Operations::OperationDescriptor>& ops_to_apply,
    const std::string& original_path,
    const std::optional<UpdateCallback>& callback
    )
{
    std::ostringstream oss;
    oss << std::hex << std::this_thread::get_id();
    std::string thread_id_str = oss.str();
    spdlog::debug("StateImageManager::performUpdate: Started on thread {}.", thread_id_str);

    auto backend = CaptureMoment::Core::Config::AppConfig::instance().getProcessingBackend();
    spdlog::debug("StateImageManager::performUpdate: Using backend: {}",
                  (backend == Common::MemoryType::CPU_RAM) ? "CPU" : "GPU");

    // Create the new working image based on the selected backend
    std::unique_ptr<ImageProcessing::IWorkingImageHardware> new_working_image;
    if (backend == Common::MemoryType::CPU_RAM) {
        new_working_image = std::make_unique<ImageProcessing::WorkingImageCPU_Halide>();
    } else {
        new_working_image = std::make_unique<ImageProcessing::WorkingImageGPU_Halide>();
    }

    bool success = false;

    spdlog::trace("StateImageManager::performUpdate: Using original path: '{}'", original_path);

    auto original_tile = m_source_manager->getTile(0, 0, m_source_manager->width(), m_source_manager->height());
    if (!original_tile)
    {
        spdlog::error("StateImageManager::performUpdate (thread {}): Failed to get original tile from SourceManager.", thread_id_str);
        success = false;
    }
    else
    {
        // Load the original image data into the new working image buffer
        if (!new_working_image->updateFromCPU(*original_tile)) {
            spdlog::error("StateImageManager::performUpdate (thread {}): Failed to update working image from CPU source.", thread_id_str);
            success = false;
        } else {
            // Apply the sequence of operations using the pipeline
            if (!m_operation_pipeline->applyOperations(*new_working_image, ops_to_apply, *m_operation_factory)) {
                spdlog::error("StateImageManager::performUpdate (thread {}): OperationPipeline::applyOperations failed.", thread_id_str);
                success = false;
            } else {
                spdlog::debug("StateImageManager::performUpdate (thread {}): OperationPipeline applied {} operations successfully.", thread_id_str, ops_to_apply.size());
                success = true;
            }
        }
    }

    {
        std::lock_guard lock(m_mutex);
        if (success) {
            m_working_image = std::move(new_working_image);
            spdlog::info("StateImageManager::performUpdate (thread {}): Working image updated successfully.", thread_id_str);
        } else {
            spdlog::warn("StateImageManager::performUpdate (thread {}): Keeping previous working image due to pipeline failure.", thread_id_str);
        }
        m_is_updating.store(false);
    }

    if (callback.has_value()) {
        callback.value()(success);
    }

    spdlog::debug("StateImageManager::performUpdate: Finished on thread {}.", thread_id_str);
    return success;
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

} // namespace CaptureMoment::Core::Managers
