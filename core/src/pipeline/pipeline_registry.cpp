/**
 * @file pipeline_registry.cpp
 * @brief Implementation of PipelineRegistry.
 * @author CaptureMoment Team
 * @date 2026
 */

#include "pipeline/pipeline_registry.h"
#include "pipeline/operations/operation_pipeline_executor_cpu.h"
#include "pipeline/operations/operation_pipeline_executor_gpu.h"
#include "pipeline/pipeline_builder.h"

#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::Pipeline {

void PipelineRegistry::registerAll() {
    spdlog::info("PipelineRegistry: Registering all pipeline executors");

    registerHalideExecutors();
    // registerAIExecutors(builder); // TODO: Enable when AI manager is ready

    spdlog::info("PipelineRegistry: All pipeline executors registered");
}

void PipelineRegistry::registerHalideExecutors() {
    spdlog::debug("PipelineRegistry: Registering Halide operation executors");

    // Halide Fused Operations
    const auto backend {Config::AppConfig::instance().getProcessingBackend()};

    if (backend == Common::MemoryType::GPU_MEMORY) {
        PipelineBuilder::registerCreator(PipelineType::HalideOperation, []() {
            return std::make_unique<OperationPipelineExecutorGPU>();
        });
        spdlog::trace("PipelineRegistry: Registered HalideOperation Executor (GPU)");
    } else {
        PipelineBuilder::registerCreator(PipelineType::HalideOperation, []() {
            return std::make_unique<OperationPipelineExecutorCPU>();
        });
        spdlog::trace("PipelineRegistry: Registered HalideOperation Executor (CPU)");
    }
}

void PipelineRegistry::registerAIExecutors() {
    spdlog::debug("PipelineRegistry: Registering AI/Computer Vision executors");

    // Exemple: Sky Replacement
    // PipelineBuilder::registerCreator(PipelineType::SkyAI, []() {
    //     return std::make_unique<SkyPipelineExecutor>();
    // });
    // spdlog::trace("PipelineRegistry: Registered SkyAI Executor");
}

} // namespace CaptureMoment::Core::Pipeline
