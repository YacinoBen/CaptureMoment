/**
 * @file pipeline_context.cpp
 * @brief Implementation of PipelineContext.
 * @author CaptureMoment Team
 * @date 2026
 */

#include "pipeline/pipeline_context.h"
#include "pipeline/pipeline_registry.h"

#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::Pipeline {

PipelineContext::PipelineContext()
{
    spdlog::info("PipelineContext::PipelineContext: Initializing Context...");

    // 1. Create the unique instance of the Builder
    m_builder = std::make_unique<PipelineBuilder>();

    // 2. Register all available pipeline types in this builder
    PipelineRegistry::registerAll(*m_builder);

    // 3. Instantiate concrete managers (Halide only for now)
    m_halide_manager = std::make_unique<Strategies::PipelineHalideOperationManager>(*m_builder);

    if (!m_halide_manager) {
        spdlog::error("PipelineContext::PipelineContext: Failed to build Halide Manager.");
    }

    spdlog::debug("PipelineContext::PipelineContext: Context initialized with Builder and Managers.");
    spdlog::debug("PipelineContext::PipelineContext: Builder and Managers are ready for use.");

     // Note: The actual initialization of managers with operations is done later by StateImageManager
     // when it has the list of operations ready. This allows the Context to be flexible and not
     // tied to a specific set of operations at construction time.

    spdlog::info("PipelineContext::PipelineContext: Initialization complete.");
}

} // namespace CaptureMoment::Core::Pipeline
