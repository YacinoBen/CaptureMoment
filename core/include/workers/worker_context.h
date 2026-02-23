/**
 * @file worker_context.h
 * @brief Container for worker infrastructure (Builder and Workers).
 *
 * @details
 * This class acts as a service locator for the task processing subsystem.
 * It owns the global `WorkerBuilder` and instances of `IWorkerRequest`.
 * It does NOT manage image state or processing data.
 *
 * **Responsibilities:**
 * - Owns and initializes the `WorkerBuilder` registry.
 * - Owns and initializes `IWorkerRequest` instances (e.g., Halide).
 * - Provides access to workers via references.
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "workers/worker_builder.h"
#include "workers/halide/halide_operation_worker.h"
#include "workers/worker_type.h"

#include <memory>

namespace CaptureMoment::Core {
namespace Workers {

/**
 * @class WorkerContext
 * @brief Central container for worker infrastructure.
 *
 * @details
 * This class is intended to be a member of a higher-level manager (e.g., StateImageManager).
 * It ensures that the heavy setup (Builder registration, Worker instantiation) happens once.
 * It exposes workers directly so the owner can trigger execution in any order.
 */
class WorkerContext {
public:
    /**
     * @brief Constructor.
     *
     * @details
     * Initializes the global `WorkerBuilder` and creates the worker instances.
     */
    explicit WorkerContext();

    /**
     * @brief Destructor.
     */
    ~WorkerContext() = default;

    // Disable copy and move for the Context itself to ensure single ownership of the Builder
    WorkerContext(const WorkerContext&) = delete;
    WorkerContext& operator=(const WorkerContext&) = delete;
    WorkerContext(WorkerContext&&) = delete;
    WorkerContext& operator=(WorkerContext&&) = delete;

    /**
     * @brief Gets a reference to the Halide Operation Worker.
     *
     * @details
     * Returns a non-const reference to allow the owner to call `execute()`.
     * The Context retains ownership via `unique_ptr`.
     *
     * @return Reference to the Halide worker.
     */
    [[nodiscard]] HalideOperationWorker& getHalideOperationWorker() noexcept {
        return *m_halide_worker;
    }

    /**
     * @brief Const overload for read-only access.
     */
    [[nodiscard]] const HalideOperationWorker& getHalideOperationWorker() const noexcept {
        return *m_halide_worker;
    }

private:
    /**
     * @brief The single global builder instance.
     * @details
     * Used to register and retrieve worker types.
     */
    std::unique_ptr<WorkerBuilder> m_builder;

    /**
     * @brief The specific worker for Halide adjustments.
     */
    std::unique_ptr<HalideOperationWorker> m_halide_worker;
};

} // namespace Workers

} // namespace CaptureMoment::Core
