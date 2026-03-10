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
    : m_state_manager(std::make_unique<Managers::StateImageManager>())
{
    spdlog::debug("PhotoEngine: Constructed with StateImageManager.");
}

std::expected<void, ErrorHandling::CoreError> PhotoEngine::loadImage(std::string_view path)
{
       if (!m_state_manager) {
        spdlog::error("PhotoEngine::loadImage: StateImageManager is null.");
        return std::unexpected(ErrorHandling::CoreError::Unexpected);
    }

    // 1. Load the image via StateImageManager (which internally uses SourceManager)
    // This also updates the internal state and prepares the working image.
    // The StateImageManager::loadImage method returns a boolean, so we check for success.
    if (!m_state_manager->loadImage(path)) {
        spdlog::error("PhotoEngine::loadImage: Failed to load image '{}'.", path);
        return std::unexpected(ErrorHandling::CoreError::IOError);
    }

    // 2. Trigger initial processing (Reset to Original) to have a ready-to-display image.
    auto process_result = m_state_manager->resetToOriginal();

    if (!process_result) {
        return std::unexpected(process_result.error());
    }

    spdlog::info("PhotoEngine: Loaded image '{}' successfully.", path);
    return {};
}

std::expected<void, ErrorHandling::CoreError> PhotoEngine::commitWorkingImageToSource()
{
    if (!m_state_manager) {
        return std::unexpected(ErrorHandling::CoreError::Unexpected);
    }
    return m_state_manager->commitWorkingImageToSource();
}

void PhotoEngine::resetWorkingImage()
{
    if (!m_state_manager) return;

    spdlog::debug("PhotoEngine: Resetting working image.");
    (void) m_state_manager->resetToOriginal();
}

Common::ImageDim PhotoEngine::width() const noexcept
{
    return m_state_manager->getSourceWidth();
}

Common::ImageDim PhotoEngine::height() const noexcept
{
    return m_state_manager->getSourceHeight();
}

Common::ImageChan PhotoEngine::channels() const noexcept
{
    return m_state_manager->getSourceChannels();
}

std::future<bool> PhotoEngine::applyOperations(std::vector<Operations::OperationDescriptor>&& ops)
{
    if (!m_state_manager) {
        spdlog::error("PhotoEngine::applyOperations: StateImageManager is null.");
        return std::async(std::launch::deferred, []() { return false; });
    }

    spdlog::info("PhotoEngine::applyOperations: Received {} operations.", ops.size());
    return m_state_manager->applyOperations(std::move(ops));
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
