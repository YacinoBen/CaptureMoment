/**
 * @file photo_engine.cpp
 * @brief Implementation of PhotoEngine
 * @author CaptureMoment Team
 * @date 2025
 */

#include "engine/photo_engine.h"
#include "managers/state_image_manager.h"
#include "operations/operation_registry.h"

#include <string_view>
#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::Engine {

// Constructs a PhotoEngine instance with required dependencies.
PhotoEngine::PhotoEngine()
{
    m_source_manager = std::make_shared<Managers::SourceManager>();
    m_operation_factory = std::make_shared<Operations::OperationFactory>();
    m_pipeline_builder = std::make_shared<Pipeline::OperationPipelineBuilder>();

    m_state_manager = std::make_unique<Managers::StateImageManager>(
        m_source_manager,
        m_pipeline_builder
        );


    Core::Operations::OperationRegistry::registerAll(*m_operation_factory);
    spdlog::debug("PhotoEngine: Constructed with StateImageManager using fused pipeline.");
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
    bool update_success = update_future.get();
    if (!update_success) {
        spdlog::error("PhotoEngine::loadImage: StateImageManager::requestUpdate reported failure.");
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

    auto* current_working_image_hw = m_state_manager->getWorkingImage();
    if (!current_working_image_hw) {
        spdlog::error("PhotoEngine::commitWorkingImageToSource: No valid working image to commit.");
        return false;
    }

    auto cpu_copy = current_working_image_hw->exportToCPUCopy();
    if (!cpu_copy) {
        spdlog::error("PhotoEngine::commitWorkingImageToSource: Failed to export working image to CPU copy.");
        return false;
    }

    return m_source_manager->setTile(*cpu_copy);
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
    }

    for (const auto& op : ops) {
        if (!m_state_manager->addOperation(op)) {
            spdlog::warn("PhotoEngine::applyOperations: StateImageManager::addOperation failed for operation '{}'.", op.name);
        }
    }

    auto update_future = m_state_manager->requestUpdate();
    bool update_success = update_future.get();
    if (!update_success) {
        spdlog::error("PhotoEngine::applyOperations: StateImageManager::requestUpdate reported failure.");
    } else {
        spdlog::debug("PhotoEngine::applyOperations: StateImageManager::requestUpdate completed successfully.");
    }
}

// Gets the current working image from StateImageManager.
ImageProcessing::IWorkingImageHardware* PhotoEngine::getWorkingImage() const
{
    if (!m_state_manager) {
        spdlog::warn("PhotoEngine::getWorkingImage: StateImageManager is null, returning nullptr.");
        return nullptr;
    }
    return m_state_manager->getWorkingImage();
}

std::shared_ptr<Common::ImageRegion> PhotoEngine::getWorkingImageAsRegion() const
{
    if (!m_state_manager) {
        spdlog::warn("PhotoEngine::getWorkingImageAsRegion: StateImageManager is null, returning nullptr.");
        return nullptr;
    }

    auto* working_image_hw = m_state_manager->getWorkingImage();
    if (!working_image_hw) {
        spdlog::warn("PhotoEngine::getWorkingImageAsRegion: No valid working image hardware available.");
        return nullptr;
    }

    auto cpu_copy = working_image_hw->exportToCPUCopy();
    if (!cpu_copy) {
        spdlog::error("PhotoEngine::getWorkingImageAsRegion: Failed to export working image to CPU copy.");
        return nullptr;
    }

    return cpu_copy;
}

} // namespace CaptureMoment::Core::Engine
