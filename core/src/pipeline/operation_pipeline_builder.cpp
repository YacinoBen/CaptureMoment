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

std::unique_ptr<IPipelineExecutor> OperationPipelineBuilder::build(
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

std::unique_ptr<IPipelineExecutor> OperationPipelineBuilder::build(
    std::vector<Operations::OperationDescriptor>&& operations,
    const Operations::OperationFactory& factory
    )
{
    spdlog::info("[OperationPipelineBuilder::build] Starting build for {} operations (moved)...", operations.size());

    if (operations.empty())
    {
        spdlog::info("[OperationPipelineBuilder::build] No operations provided. Returning null executor.");
        return nullptr;
    }

    try {
        // The executor will compile the fused pipeline graph once in its constructor
        // We use std::move(operations) here, but the executor will make a copy internally if needed
        return std::make_unique<OperationPipelineExecutor>(std::move(operations), factory);

    } catch (const std::exception& e) {
        spdlog::error("[OperationPipelineBuilder::build] Exception occurred during pipeline construction: {}", e.what());
        return nullptr;
    }
}

} // namespace CaptureMoment::Core::Pipeline
