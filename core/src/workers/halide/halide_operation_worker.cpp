/**
 * @file halide_operation_worker.cpp
 * @brief Implementation of HalideOperationWorker.
 * @author CaptureMoment Team
 * @date 2026
 */

#include "workers/halide/halide_operation_worker.h"
#include "strategies/pipeline/pipeline_halide_operation_manager.h"

#include <spdlog/spdlog.h>
#include <future>

namespace CaptureMoment::Core {
namespace Workers {

std::future<bool> HalideOperationWorker::execute(
    Pipeline::PipelineContext& context,
    ImageProcessing::IWorkingImageHardware& working_image)
{
    spdlog::trace("[HalideOperationWorker] Executing Halide pipeline.");

    // 1. Retrieve the specific manager from the context
    // We use the context to avoid passing the manager directly as a parameter.
    auto& halide_manager = context.getHalideManager();

    // 2. Wrap the execution in a future
    // Since the manager's execute() is synchronous (blocks until done),
    // we use std::async to create a future that runs this logic.
    // This satisfies the IWorkerRequest interface requirement.
    return std::async(std::launch::deferred, [&halide_manager, &working_image]() {
        bool success = halide_manager.execute(working_image);
        
        if (!success) {
            spdlog::error("[HalideOperationWorker] Execution failed.");
        }
        
        return success;
    });
}

} // namespace Workers
} // namespace CaptureMoment::Core
