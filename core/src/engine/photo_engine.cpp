/**
 * @file photo_engine.cpp
 * @brief Implementation of PhotoEngine.
 * @author CaptureMoment Team
 * @date 2025
 */

#include "engine/photo_engine.h"
#include "managers/state_image_manager.h"

#include <string_view>
#include <memory>
#include <spdlog/spdlog.h>
#include <utility>

namespace CaptureMoment::Core::Engine {

PhotoEngine::PhotoEngine()
    : m_source_manager(std::make_shared<Managers::SourceManager>())
    , m_state_manager(std::make_unique<Managers::StateImageManager>(m_source_manager))
{
    spdlog::debug("PhotoEngine: Constructed with StateImageManager.");
}

std::expected<void, ErrorHandling::CoreError> PhotoEngine::loadImage(std::string_view path)
{
    if (!m_source_manager) {
        spdlog::error("PhotoEngine::loadImage: SourceManager is null.");
        return std::unexpected(ErrorHandling::CoreError::Unexpected);
    }

    // 1. Load the file into SourceManager
    auto load_result = m_source_manager->loadFile(path);
    if (!load_result) {
        return std::unexpected(load_result.error());
    }

    // 2. Initialize StateImageManager with the path
    if (!m_state_manager->setOriginalImageSource(std::string(path))) {
        spdlog::error("PhotoEngine::loadImage: Failed to set image source in StateImageManager.");
        return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
    }

    // 3. Perform initial update synchronously to ensure the image is ready
    auto update_future = m_state_manager->requestUpdate();
    bool update_success = update_future.get();

    if (!update_success) {
        spdlog::error("PhotoEngine::loadImage: Initial update failed.");
        return std::unexpected(ErrorHandling::CoreError::AllocationFailed);
    }

    spdlog::info("PhotoEngine: Loaded image '{}'.", path);
    return {};
}

std::expected<void, ErrorHandling::CoreError> PhotoEngine::commitWorkingImageToSource()
{
    if (!m_source_manager || !m_state_manager) {
        spdlog::error("PhotoEngine::commitWorkingImageToSource: Managers are null.");
        return std::unexpected(ErrorHandling::CoreError::Unexpected);
    }

    // 1. Retrieve the current working image
    auto working_image_hw = m_state_manager->getWorkingImage();
    if (!working_image_hw) {
        spdlog::error("PhotoEngine::commitWorkingImageToSource: No working image available.");
        return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
    }

    // 2. Export to CPU memory
    auto cpu_copy_result = working_image_hw->exportToCPUCopy();
    if (!cpu_copy_result) {
        spdlog::error("PhotoEngine::commitWorkingImageToSource: CPU export failed: {}",
                       ErrorHandling::to_string(cpu_copy_result.error()));
        return std::unexpected(cpu_copy_result.error());
    }

    std::unique_ptr<Common::ImageRegion> cpu_copy = std::move(cpu_copy_result.value());

    // 3. Write back to SourceManager
    if (!m_source_manager->setTile(*cpu_copy)) {
        spdlog::error("PhotoEngine::commitWorkingImageToSource: Write to source failed.");
        return std::unexpected(ErrorHandling::CoreError::IOError);
    }

    spdlog::info("PhotoEngine: Changes committed to source.");
    return {};
}

void PhotoEngine::resetWorkingImage()
{
    if (!m_state_manager) return;

    spdlog::debug("PhotoEngine: Resetting working image.");
    (void) m_state_manager->resetToOriginal();
}

int PhotoEngine::width() const noexcept
{
    return m_source_manager ? m_source_manager->width() : 0;
}

int PhotoEngine::height() const noexcept
{
    return m_source_manager ? m_source_manager->height() : 0;
}

int PhotoEngine::channels() const noexcept
{
    return m_source_manager ? m_source_manager->channels() : 0;
}

void PhotoEngine::applyOperations(const std::vector<Operations::OperationDescriptor>& ops)
{
    if (!m_state_manager) {
        spdlog::error("PhotoEngine::applyOperations: StateImageManager is null.");
        return;
    }

    spdlog::debug("PhotoEngine: Applying {} operations.", ops.size());

    // Replace current state with the new list
    // Note: resetToOriginal clears the list, then we add new ops.
    (void) m_state_manager->resetToOriginal();
    for (const auto& op : ops) {
        (void) m_state_manager->addOperation(op);
    }

    // Trigger asynchronous processing
     (void) m_state_manager->requestUpdate();
}

std::shared_ptr<ImageProcessing::IWorkingImageHardware> PhotoEngine::getWorkingImage() const
{
    return m_state_manager ? m_state_manager->getWorkingImage() : nullptr;
}

std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError> PhotoEngine::getWorkingImageAsRegion() const
{
    if (!m_state_manager) {
        return std::unexpected(ErrorHandling::CoreError::Unexpected);
    }

    auto working_image_hw = m_state_manager->getWorkingImage();
    if (!working_image_hw) {
        return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
    }

    // Export HW image to CPU
    auto cpu_copy = working_image_hw->exportToCPUCopy();
    if (!cpu_copy) {
        return std::unexpected(cpu_copy.error());
    }

    return cpu_copy;
}

} // namespace CaptureMoment::Core::Engine
