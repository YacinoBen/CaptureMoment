/**
 * @file pipeline_registry.h
 * @brief Central registry for all pipeline executors.
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

namespace CaptureMoment::Core {

namespace Pipeline {

class PipelineBuilder;

/**
 * @class PipelineRegistry
 * @brief Manages registration of all available pipeline executors (Strategies).
 *
 * @details
 * Provides a centralized point to register different types of image processing backends
 * (e.g., Halide for adjustments, OpenCV/AI for effects). Extensible for adding new
 * pipeline categories.
 */
class PipelineRegistry {
public:
    /**
     * @brief Register all available pipeline executors.
     */
    static void registerAll();

private:
    /**
     * @brief Register Halide-based operation executors (Brightness, Contrast, etc.).
     */
    static void registerHalideExecutors();

    /**
     * @brief Register AI/OpenCV based executors (Sky replacement, etc.).
     */
    static void registerAIExecutors();
};

} // namespace Pipeline

} // namespace CaptureMoment::Core
