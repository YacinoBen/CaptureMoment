/**
 * @file working_image_factory.cpp
 * @brief Implementation of WorkingImageFactory.
 * @details Registry-based factory logic.
 * @author CaptureMoment Team
 * @date 2026
 */

#include "image_processing/factories/working_image_factory.h"
#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::ImageProcessing {

std::unique_ptr<IWorkingImageHardware> WorkingImageFactory::create(
    Common::MemoryType backend,
    const Common::ImageRegion& source_image
    )
{
    // Look up creator for the requested backend
    auto it = s_registry.find(backend);

    if (it == s_registry.end()) {
        spdlog::error("[WorkingImageFactory] No creator registered for backend type {}. Unable to create working image.",
                      static_cast<int>(backend));
        return nullptr;
    }

    // Execute creator function
    try {
        auto creator = it->second;
        return creator(source_image);
    } catch (const std::exception& e) {
        spdlog::critical("[WorkingImageFactory] Exception thrown by creator for backend {}: {}",
                         static_cast<int>(backend), e.what());
        return nullptr;
    }
}

void WorkingImageFactory::registerCreator(
    Common::MemoryType type,
    CreatorFunction creator
    )
{
    if (s_registry.contains(type)) {
        spdlog::warn("[WorkingImageFactory] Overriding existing creator for backend type {}.", static_cast<int>(type));
    }
    s_registry[type] = std::move(creator);
}

} // namespace CaptureMoment::Core::ImageProcessing
