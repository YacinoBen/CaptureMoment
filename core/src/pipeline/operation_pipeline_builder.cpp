/**
 * @file operation_pipeline_builder.cpp
 * @brief Implementation of OperationPipelineBuilder.
 * @author CaptureMoment Team
 * @date 2026
 */

#include "pipeline/operation_pipeline_builder.h"
#include "pipeline/operation_pipeline_executor.h"

#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::Pipeline {

/**
 * @brief Static method to build a fused pipeline executor.
 *
 * @details
 * This method creates an `OperationPipelineExecutor` instance.
 * The executor will compile the fused Halide pipeline once (in its constructor) and cache it
 * for efficient reuse during every `execute` call.
 *
 * @param operations A vector of `OperationDescriptor` objects defining the sequence.
 * @param factory The `OperationFactory` used to retrieve operation-specific fusion logic.
 * @return A unique pointer to an `IPipelineExecutor` object.
 *         Returns nullptr if `operations` is empty or an exception occurs.
 */

std::unique_ptr<IPipelineExecutor> build(
    const std::vector<Operations::OperationDescriptor>& operations,
    const Operations::OperationFactory& factory
    )
{
    spdlog::info("[OperationPipelineBuilder::build] Starting build for {} operations...", operations.size());

    if (operations.empty())
    {
        spdlog::info("[OperationPipelineBuilder::build] No operations provided. Returning null executor.");
        return nullptr;
    }

    try {
        // The executor will compile the fused pipeline graph once in its constructor
        return std::make_unique<OperationPipelineExecutor>(operations, factory);

    } catch (const std::exception& e) {
        spdlog::error("[OperationPipelineBuilder::build] Exception occurred during pipeline construction: {}", e.what());
        return nullptr;
    }
}

} // namespace CaptureMoment::Core::Pipeline
