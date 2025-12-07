/**
 * @file pipeline_engine.cpp
 * @brief Implementation of PipelineEngine
 * @author CaptureMoment Team
 * @date 2025
 */

#include "engine/pipeline_engine.h"
#include "managers/source_manager.h"
#include "operations/operation_factory.h"
#include "operations/i_operation.h"
#include "common/image_region.h"
#include "operations/operation_descriptor.h"
#include <spdlog/spdlog.h>

namespace CaptureMoment {

PipelineEngine::PipelineEngine(SourceManager& source, const OperationFactory& factory)
    : m_source(source), m_factory(factory)
{
    spdlog::debug("PipelineEngine: Instance created");
}

bool PipelineEngine::processRegion(
    int x, int y, int width, int height,
    const std::vector<OperationDescriptor>& operations
) {
    // 1. Get tile from SourceManager (all preprocessing done here)
    auto tile = m_source.getTile(x, y, width, height);
    
    if (!tile) {
        spdlog::warn("PipelineEngine::processRegion: Failed to get tile at ({}, {}) size {}x{}",
                     x, y, width, height);
        return false;
    }

    spdlog::info("PipelineEngine::processRegion: Processing tile at ({}, {}) {}x{} with {} operations",
                 x, y, width, height, operations.size());

    // 2. Apply each operation sequentially (Strategy Pattern)
    for (const auto& descriptor : operations) {
        if (!descriptor.enabled) {
            spdlog::trace("PipelineEngine::processRegion: Skipping disabled operation '{}'",
                          descriptor.name);
            continue;
        }

        // Factory creates the concrete operation (Dependency Injection)
        auto operation = m_factory.create(descriptor);
        
        if (!operation) {
            spdlog::error("PipelineEngine::processRegion: Failed to create operation '{}'",
                          descriptor.name);
            return false;
        }

        spdlog::debug("PipelineEngine::processRegion: Executing operation '{}'",
                      descriptor.name);

        // Execute operation on tile (Strategy Pattern - polymorphism)
        if (!operation->execute(*tile, descriptor)) {
            spdlog::error("PipelineEngine::processRegion: Operation '{}' failed",
                          descriptor.name);
            return false;
        }
    }

    // 3. Write tile back (optional: for in-place editing)
    if (!writeTileBack(x, y, *tile)) {
        spdlog::warn("PipelineEngine::processRegion: Failed to write tile back");
        return false;
    }

    spdlog::trace("PipelineEngine::processRegion: Tile processing completed");
    return true;
}

bool PipelineEngine::writeTileBack(int x, int y, const ImageRegion& tile) {
    spdlog::trace("PipelineEngine::writeTileBack: Tile at ({}, {}) ready for use", x, y);
    return true;
}

} // namespace CaptureMoment