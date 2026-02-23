/**
 * @file pipeline_registry.h
 * @brief Central registry for all pipeline executors.
 *
 * @details
 * This class manages the registration of available `IPipelineExecutor` implementations
 * with the central `PipelineBuilder`.
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "pipeline/pipeline_builder.h"

namespace CaptureMoment::Core {

namespace Pipeline {

// Forward declaration
class PipelineBuilder;

/**
 * @class PipelineRegistry
 * @brief Manages registration of all available pipeline executors (Strategies).
 *
 * @details
 * Provides a centralized point to register different types of image processing backends
 * (e.g., Halide for adjustments, OpenCV/AI for effects). Extensible for adding new
 * pipeline categories.
 *
 * Usage:
 * @code
 * Pipeline::PipelineBuilder builder;
 * PipelineRegistry::registerAll(builder);
 * @endcode
 */
class PipelineRegistry {
public:
    /**
     * @brief Register all available pipeline executors.
     *
     * @param builder The reference to the PipelineBuilder to populate.
     */
    static void registerAll(PipelineBuilder& builder);

private:
    /**
     * @brief Register Halide-based operation executors (Brightness, Contrast, etc.).
     *
     * @param builder The reference to the PipelineBuilder.
     */
    static void registerHalideExecutors(PipelineBuilder& builder);

    /**
     * @brief Register AI/OpenCV based executors (Sky replacement, etc.).
     *
     * @param builder The reference to the PipelineBuilder.
     */
    static void registerAIExecutors(PipelineBuilder& builder);
};

} // namespace Pipeline

} // namespace CaptureMoment::Core
