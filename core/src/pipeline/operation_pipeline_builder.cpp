/**
 * @file operation_pipeline_builder.cpp
 * @brief Implementation of OperationPipelineBuilder
 * @author CaptureMoment Team
 * @date 2026
 */

#include "pipeline/operation_pipeline_builder.h"
#include "pipeline/operation_pipeline_executor.h"
#include "operations/operation_factory.h"
#include <spdlog/spdlog.h>
#include <memory>
#include <vector>

namespace CaptureMoment::Core::Pipeline {

/**
 * @brief Static method to build a fused Halide pipeline for the given adjustment operations.
 *
 * This method creates an OperationPipelineExecutor instance that encapsulates a compiled
 * Halide pipeline combining all the specified operations. The pipeline is constructed
 * once and cached within the executor for efficient reuse during execution.
 *
 * @param operations A vector of OperationDescriptor objects defining the sequence of adjustment operations to apply and fuse.
 * @param factory The OperationFactory instance used to retrieve operation-specific fusion logic required for pipeline construction.
 * @return A unique pointer to an IPipelineExecutor object, which encapsulates the compiled pipeline and provides an execute method.
 *         Returns nullptr if the pipeline construction fails.
 */
std::unique_ptr<IPipelineExecutor> OperationPipelineBuilder::build(
    const std::vector<Operations::OperationDescriptor>& operations,
    const Operations::OperationFactory& factory
    ) {
    spdlog::info("OperationPipelineBuilder::build: Starting build for {} operations.", operations.size());

    if (operations.empty()) {
        spdlog::info("OperationPipelineBuilder::build: No operations provided. Returning executor with empty pipeline.");
        return std::make_unique<OperationPipelineExecutor>(operations, factory);
    }

    try {
        // Create an OperationPipelineExecutor instance with the operations and factory.
        // The executor will compile the fused pipeline once in its constructor and cache it
        // for efficient reuse during subsequent executions.
        return std::make_unique<OperationPipelineExecutor>(operations, factory);

    } catch (const std::exception& e) {
        spdlog::error("OperationPipelineBuilder::build: Exception occurred during pipeline construction: {}", e.what());
        return nullptr;
    }
}

} // namespace CaptureMoment::Core::Pipeline
