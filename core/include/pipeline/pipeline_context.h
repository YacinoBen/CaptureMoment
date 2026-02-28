/**
 * @file pipeline_context.h
 * @brief Container for pipeline infrastructure (Builder and Managers).
 *
 * @details
 * This class acts as a service locator for the image processing subsystem.
 * It owns the global `PipelineBuilder` and instances of `IPipelineManager`.
 * It does NOT manage image state or operations.
 *
 * **Responsibilities:**
 * - Owns and initializes the `PipelineBuilder` registry.
 * - Owns and initializes `IPipelineManager` instances (e.g., Halide).
 * - Injects the builder dependency into managers.
 * - Provides access to managers via references.
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "pipeline/pipeline_builder.h"
#include "strategies/pipeline/pipeline_halide_operation_manager.h"
#include "operations/operation_factory.h"
#include "operations/operation_descriptor.h"

#include <memory>
#include <vector>

namespace CaptureMoment::Core {
namespace Pipeline {

/**
 * @class PipelineContext
 * @brief Central container for pipeline infrastructure.
 *
 * @details
 * This class is intended to be a member of a higher-level manager (e.g., StateImageManager).
 * It ensures that the heavy setup (Builder registration, Manager instantiation) happens once.
 * It exposes managers directly so the owner can trigger updates and execution in any order.
 */

class PipelineContext {
public:
    /**
     * @brief Constructor.
     *
     * @details
     * Initializes the global `PipelineBuilder` and creates the manager instances.
     * Managers are created empty; they must be initialized via their `init()` method
     * by the owner of this context when operations are available.
     */
    explicit PipelineContext();

    /**
     * @brief Destructor.
     */
    ~PipelineContext() = default;

    // Disable copy and move for the Context itself to ensure single ownership of the Builder
    PipelineContext(const PipelineContext&) = delete;
    PipelineContext& operator=(const PipelineContext&) = delete;
    PipelineContext(PipelineContext&&) = delete;
    PipelineContext& operator=(PipelineContext&&) = delete;

    /**
     * @brief Gets a reference to the Halide Operation Manager.
     *
     * @details
     * Returns a non-const reference to allow the owner to call `init()` and `execute()`.
     * The Context retains ownership via `unique_ptr`.
     *
     * @return Reference to the Halide manager.
     */
    [[nodiscard]] Strategies::PipelineHalideOperationManager& getHalideManager() noexcept {
        return *m_halide_manager;
    }

    /**
     * @brief Const overload for read-only access.
     */
    [[nodiscard]] const Strategies::PipelineHalideOperationManager& getHalideManager() const noexcept {
        return *m_halide_manager;
    }

private:
    /**
     * @brief The specific manager for Halide adjustments.
     */
    std::unique_ptr<Strategies::PipelineHalideOperationManager> m_halide_manager;
};

} // namespace Pipeline

} // namespace CaptureMoment::Core
