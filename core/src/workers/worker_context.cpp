/**
 * @file worker_context.cpp
 * @brief Implementation of WorkerContext.
 * @author CaptureMoment Team
 * @date 2026
 */

#include "workers/worker_context.h"
#include "workers/worker_registry.h"

#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::Workers {

WorkerContext::WorkerContext()
{
    spdlog::info("WorkerContext::WorkerContext: Initializing Context...");

    // 1. Create the unique instance of the Builder
    m_builder = std::make_unique<WorkerBuilder>();

    // 2. Register all available worker types in this builder
    WorkerRegistry::registerAll(*m_builder);

    // 3. Instantiate concrete workers (Halide only for now)
    // Note: We assume WorkerBuilder::build returns the correct concrete type.
    // For Halide, we create the specific worker instance.
    m_halide_worker = std::make_unique<HalideOperationWorker>();

    if (!m_halide_worker) {
        spdlog::error("WorkerContext::WorkerContext: Failed to build Halide Worker.");
    }

    spdlog::debug("WorkerContext::WorkerContext: Context initialized with Builder and Workers.");
    spdlog::debug("WorkerContext::WorkerContext: Builder and Workers are ready for use.");

    // Note: The actual execution with operations is done by the caller (StateImageManager)
    // passing the context and data to the worker.

    spdlog::info("WorkerContext::WorkerContext: Initialization complete.");
}

} // namespace CaptureMoment::Core::Workers
