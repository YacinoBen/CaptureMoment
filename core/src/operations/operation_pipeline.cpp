#include "operations/operation_pipeline.h"
#include "operations/operation_factory.h"
#include "operations/operation_descriptor.h"
#include "operations/interfaces/i_operation.h"
#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::Operations {

std::expected<void, ErrorHandling::CoreError> OperationPipeline::applyOperations(
    ImageProcessing::IWorkingImageHardware& working_image,
    const std::vector<OperationDescriptor>& operations,
    const OperationFactory& factory
    )
{
    spdlog::info("OperationPipeline::applyOperations: Starting with {} operations", operations.size());

    for (const auto& descriptor : operations)
    {
        if (!descriptor.enabled) {
            spdlog::trace("OperationPipeline::applyOperations: Skipping disabled operation '{}'", descriptor.name);
            continue;
        }

        spdlog::info("OperationPipeline::applyOperations: Creating operation '{}'", descriptor.name);

        auto result_op = factory.create(descriptor);
        if (!result_op) {
            spdlog::error("OperationPipeline::applyOperations: Failed to create operation '{}'", descriptor.name);
            return std::unexpected(result_op.error());
        }

        auto& operation = result_op.value();

        spdlog::debug("OperationPipeline::applyOperations: Executing operation '{}'", descriptor.name);

        if (!operation->execute(working_image, descriptor)) {
            spdlog::error("OperationPipeline::applyOperations: Operation '{}' failed", descriptor.name);
            return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
        }

        spdlog::info("OperationPipeline::applyOperations: Operation '{}' completed", descriptor.name);
    }

    spdlog::info("OperationPipeline::applyOperations: All operations completed successfully");
    return {};
}

} // namespace CaptureMoment::Core::Operations
