/**
 * @file operation_pipeline.cpp
 * @brief Implementation of OperationPipeline
 * @author CaptureMoment Team
 * @date 2026
 */

#include "operations/operation_pipeline.h"
#include "operations/operation_factory.h"
#include "operations/operation_descriptor.h"
#include "operations/interfaces/i_operation.h"
#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::Operations {

bool OperationPipeline::applyOperations(
    ImageProcessing::IWorkingImageHardware& working_image,
    const std::vector<OperationDescriptor>& operations,
    const OperationFactory& factory
    )
{
    spdlog::info("OperationPipeline::applyOperations: Starting with {} operations", operations.size());
    for (const auto& descriptor : operations) {
        if (!descriptor.enabled) {
            spdlog::trace("OperationPipeline::applyOperations: Skipping disabled operation '{}'", descriptor.name);
            continue;
        }

        spdlog::info("OperationPipeline::applyOperations: Creating operation '{}'", descriptor.name);
        auto operation = factory.create(descriptor);
        if (!operation) {
            spdlog::error("OperationPipeline::applyOperations: Failed to create operation '{}'", descriptor.name);
            return false;
        }

        spdlog::debug("OperationPipeline::applyOperations: Executing operation '{}'", descriptor.name);

        if (!operation->execute(working_image, descriptor)) {
            spdlog::error("OperationPipeline::applyOperations: Operation '{}' failed", descriptor.name);
            return false;
        }
        spdlog::info("OperationPipeline::applyOperations: Operation '{}' completed", descriptor.name);
    }

    spdlog::info("OperationPipeline::applyOperations: All operations completed successfully");
    return true;
}

} // namespace CaptureMoment::Core::Operations
