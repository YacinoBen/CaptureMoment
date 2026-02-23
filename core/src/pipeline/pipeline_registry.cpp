/**
 * @file pipeline_registry.cpp
 * @brief Implementation of PipelineRegistry.
 * @author CaptureMoment Team
 * @date 2026
 */

#include "pipeline/pipeline_registry.h"
#include "pipeline/operation_pipeline_executor.h"

#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::Pipeline {

void PipelineRegistry::registerAll(PipelineBuilder& builder) {
    spdlog::info("PipelineRegistry: Registering all pipeline executors");

    registerHalideExecutors(builder);
    // registerAIExecutors(builder); // TODO: Enable when AI manager is ready

    spdlog::info("PipelineRegistry: All pipeline executors registered");
}

void PipelineRegistry::registerHalideExecutors(PipelineBuilder& builder) {
    spdlog::debug("PipelineRegistry: Registering Halide operation executors");

    // Halide Fused Operations
    builder.registerCreator(PipelineType::HalideOperation, []() {
        return std::make_unique<OperationPipelineExecutor>();
    });
    spdlog::trace("PipelineRegistry: Registered HalideOperation Executor");
}

void PipelineRegistry::registerAIExecutors(PipelineBuilder& builder) {
    spdlog::debug("PipelineRegistry: Registering AI/Computer Vision executors");

    // Exemple: Sky Replacement
    // builder.registerCreator(PipelineType::SkyAI, []() {
    //     return std::make_unique<SkyPipelineExecutor>();
    // });
    // spdlog::trace("PipelineRegistry: Registered SkyAI Executor");
}

} // namespace CaptureMoment::Core::Pipeline
