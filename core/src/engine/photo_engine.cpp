/**
 * @file photo_engine.cpp
 * @brief Implementation of PhotoEngine
 * @author CaptureMoment Team
 * @date 2025
 */

#include "engine/photo_engine.h"
#include "managers/state_image_manager.h"
#include <string_view>
#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::Engine {

// Constructs a PhotoEngine instance with required dependencies.
PhotoEngine::PhotoEngine(
    std::shared_ptr<Managers::SourceManager> source_manager,
    std::shared_ptr<Operations::OperationFactory> operation_factory,
    std::shared_ptr<Operations::OperationPipeline> operation_pipeline
    )
    : m_source_manager{std::move(source_manager)}
    , m_operation_factory{std::move(operation_factory)}
    , m_operation_pipeline{std::move(operation_pipeline)}
{
    m_state_manager = std::make_unique<Managers::StateImageManager>(
        m_source_manager,
        m_operation_pipeline,
        m_operation_factory
        );
    spdlog::debug("PhotoEngine: Constructed with StateImageManager.");
}

// Loads an image file into the engine via SourceManager and initializes StateImageManager.
bool PhotoEngine::loadImage(std::string_view path)
{
    if (!m_source_manager) {
        spdlog::error("PhotoEngine::loadImage: SourceManager is null.");
        return false;
    }

    if (!m_source_manager->loadFile(path)) {
        spdlog::error("PhotoEngine::loadImage: Failed to load file '{}' via SourceManager.", path);
        return false;
    }

    if (!m_state_manager->setOriginalImageSource(std::string(path))) {
        spdlog::error("PhotoEngine::loadImage: Failed to set original image source in StateImageManager.");
        return false;
    }

    auto update_future = m_state_manager->requestUpdate();

    // Check the result of the asynchronous update
    bool update_success = update_future.get(); // This will block until the update completes and return the bool result
    if (!update_success) {
        spdlog::error("PhotoEngine::loadImage: StateImageManager::requestUpdate reported failure.");
        // Depending on desired behavior, could return or throw here if critical.
        // For now, just log the error. The image might still be in a previous valid state.
    } else {
        spdlog::debug("PhotoEngine::loadImage: StateImageManager::requestUpdate completed successfully.");
    }

    spdlog::info("PhotoEngine::loadImage: Successfully loaded and initialized StateImageManager for '{}'.", path);
    return true;
}


// Commits the current working image managed by StateImageManager back to the source.
bool PhotoEngine::commitWorkingImageToSource()
{
    if (!m_source_manager || !m_state_manager) {
        spdlog::error("PhotoEngine::commitWorkingImageToSource: SourceManager or StateImageManager is null.");
        return false;
    }

    auto current_working_image = m_state_manager->getWorkingImage();
    if (!current_working_image) {
        spdlog::error("PhotoEngine::commitWorkingImageToSource: No valid working image to commit.");
        return false;
    }

    return m_source_manager->setTile(*current_working_image);
}

// Resets the working image state via StateImageManager.
void PhotoEngine::resetWorkingImage()
{
    if (!m_state_manager) {
        spdlog::error("PhotoEngine::resetWorkingImage: StateImageManager is null.");
        return;
    }
    spdlog::debug("PhotoEngine::resetWorkingImage: Resetting StateImageManager.");
    if (!m_state_manager->resetToOriginal()) {
        spdlog::warn("PhotoEngine::resetWorkingImage: StateImageManager::resetToOriginal failed or was ignored.");
        // Depending on desired behavior, could return or throw here if critical.
    }
}

// Gets the width of the currently loaded image via SourceManager.
int PhotoEngine::width() const noexcept
{
    if (m_source_manager) {
        return m_source_manager->width();
    }
    return 0;
}

// Gets the height of the currently loaded image via SourceManager.
int PhotoEngine::height() const noexcept
{
    if (m_source_manager) {
        return m_source_manager->height();
    }
    return 0;
}

// Gets the number of channels of the currently loaded image via SourceManager.
int PhotoEngine::channels() const noexcept
{
    if (m_source_manager) {
        return m_source_manager->channels();
    }
    return 0;
}

// Applies a sequence of operations cumulatively via StateImageManager.
void PhotoEngine::applyOperations(const std::vector<Operations::OperationDescriptor>& ops)
{
    if (!m_state_manager) {
        spdlog::error("PhotoEngine::applyOperations: StateImageManager is null.");
        return;
    }

    spdlog::debug("PhotoEngine::applyOperations: Received {} operations to apply cumulatively.", ops.size());

    if (!m_state_manager->resetToOriginal()) {
        spdlog::warn("PhotoEngine::applyOperations: StateImageManager::resetToOriginal failed or was ignored.");
        // Depending on desired behavior, could return here if critical.
    }

    for (const auto& op : ops) {
        if (!m_state_manager->addOperation(op)) {
            spdlog::warn("PhotoEngine::applyOperations: StateImageManager::addOperation failed for operation '{}'.", op.name);
            // Depending on desired behavior, could return here if critical.
        }
    }

    auto update_future = m_state_manager->requestUpdate();

    // Check the result of the asynchronous update
    bool update_success = update_future.get(); // This will block until the update completes and return the bool result
    if (!update_success) {
        spdlog::error("PhotoEngine::applyOperations: StateImageManager::requestUpdate reported failure.");
        // Depending on desired behavior, could return or throw here if critical.
        // For now, just log the error. The image might still be in a previous valid state.
    } else {
        spdlog::debug("PhotoEngine::applyOperations: StateImageManager::requestUpdate completed successfully.");
    }
}

// Gets the current working image from StateImageManager.
std::shared_ptr<Common::ImageRegion> PhotoEngine::getWorkingImage() const
{
    if (!m_state_manager) {
        spdlog::warn("PhotoEngine::getWorkingImage: StateImageManager is null, returning nullptr.");
        return nullptr;
    }
    return m_state_manager->getWorkingImage();
}

} // namespace CaptureMoment::Core::Engine
