/**
 * @file worker_registry.cpp
 * @brief Implementation of WorkerRegistry.
 * @author CaptureMoment Team
 * @date 2026
 */

#include "workers/worker_registry.h"
#include "workers/halide/halide_operation_worker.h"

#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::Workers {

void WorkerRegistry::registerAll(WorkerBuilder& builder) {
    spdlog::info("WorkerRegistry: Registering all workers");

    registerHalideWorkers(builder);
    // registerAIWorkers(builder); // TODO: Enable when AI worker is ready

    spdlog::info("WorkerRegistry: All workers registered");
}

void WorkerRegistry::registerHalideWorkers(WorkerBuilder& builder) {
    spdlog::debug("WorkerRegistry: Registering Halide workers");

    builder.registerCreator(WorkerType::HalideOperation, []() {
        return std::make_unique<HalideOperationWorker>();
    });
    spdlog::trace("WorkerRegistry: Registered HalideOperationWorker");
}

void WorkerRegistry::registerAIWorkers(WorkerBuilder& builder) {
    spdlog::debug("WorkerRegistry: Registering AI/Computer Vision workers");

    // Exemple: Sky Replacement
    // builder.registerCreator(WorkerType::SkyAI, []() {
    //     return std::make_unique<SkyReplacementWorker>();
    // });
    // spdlog::trace("WorkerRegistry: Registered SkyReplacementWorker");
}

} // namespace CaptureMoment::Core::Workers
