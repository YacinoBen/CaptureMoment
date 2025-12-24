/**
 * @file operation_pipeline.cpp
 * @brief Implementation of OperationPipeline
 * @author CaptureMoment Team
 * @date 2025
 */

#include "operations/operation_pipeline.h"
#include "operations/operation_factory.h"
#include "common/image_region.h"
#include "operations/operation_descriptor.h"
#include "operations/i_operation.h"
#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::Operations {

    // Implementation of the static applyOperations function.
    // Applies a sequence of operations to an image region.
    bool OperationPipeline::applyOperations(
        Common::ImageRegion& tile,                                  // The image region to process (modified in-place).
        const std::vector<OperationDescriptor>& operations, // The list of operations to apply.
        const OperationFactory& factory                     // The factory to create operation instances.
    ) {
        // Iterate through each operation descriptor in the list.
        spdlog::info("OperationPipeline::applyOperations: Starting with {} operations", operations.size());
        for (const auto& descriptor : operations) {
            // Skip operations that are not enabled.
            if (!descriptor.enabled) {
                spdlog::trace("PipelineEngine::applyOperations: Skipping disabled operation '{}'", descriptor.name);
                continue;
            }

            // Use the factory to create an instance of the concrete operation.
            spdlog::info("OperationPipeline::applyOperations: Creating operation '{}'", descriptor.name);
            auto operation = factory.create(descriptor);
            if (!operation) {
                spdlog::error("PipelineEngine::applyOperations: Failed to create operation '{}'", descriptor.name);
                return false; // Stop processing if operation creation fails.
            }

            spdlog::debug("PipelineEngine::applyOperations: Executing operation '{}'", descriptor.name);

            // Execute the operation on the tile.
            if (!operation->execute(tile, descriptor)) {
                spdlog::error("PipelineEngine::applyOperations: Operation '{}' failed", descriptor.name);
                return false; // Stop processing if operation execution fails.
            }
            spdlog::info("OperationPipeline::applyOperations: Operation '{}' completed", descriptor.name);
        }

        // All operations completed successfully.
        spdlog::info("OperationPipeline::applyOperations: All operations completed successfully");
        return true;
    }
    
} // namespace CaptureMoment::Core::Operations
