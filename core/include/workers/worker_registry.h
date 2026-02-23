/**
 * @file worker_registry.h
 * @brief Central registry for all processing workers.
 *
 * @details
 * This class manages the registration of available `IWorkerRequest` implementations
 * with the central `WorkerBuilder`.
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "workers/worker_builder.h"

namespace CaptureMoment::Core {

namespace Workers {

// Forward declaration
class WorkerBuilder;

/**
 * @class WorkerRegistry
 * @brief Manages registration of all available processing workers.
 *
 * @details
 * Provides a centralized point to register different types of processing workers
 * (e.g., Halide for adjustments, OpenCV/AI for effects). Extensible for adding new
 * worker categories.
 *
 * Usage:
 * @code
 * Workers::WorkerBuilder builder;
 * Workers::WorkerRegistry::registerAll(builder);
 * @endcode
 */
class WorkerRegistry {
public:
    /**
     * @brief Register all available workers.
     *
     * @param builder The reference to the WorkerBuilder to populate.
     */
    static void registerAll(WorkerBuilder& builder);

private:
    /**
     * @brief Register Halide-based workers (Adjustments).
     *
     * @param builder The reference to the WorkerBuilder.
     */
    static void registerHalideWorkers(WorkerBuilder& builder);

    /**
     * @brief Register AI/OpenCV based workers (Sky replacement, etc.).
     *
     * @param builder The reference to the WorkerBuilder.
     */
    static void registerAIWorkers(WorkerBuilder& builder);
};

} // namespace Workers

} // namespace CaptureMoment::Core
