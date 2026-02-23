/**
 * @file pipeline_type.h
 * @brief Enumeration defining supported pipeline types.
 *
 * @details
 * This file contains the `PipelineType` enum which serves as a key
 * for the PipelineBuilder registry. It allows the system to identify
 * and instantiate the correct concrete execution strategy (e.g., Halide, OpenCV, AI)
 * at runtime without hard dependencies.
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include <cstdint>

namespace CaptureMoment::Core {

namespace Pipeline {

/**
 * @enum PipelineType
 * @brief Identifies the specific technology or strategy used for image processing.
 *
 * @details
 * Each value corresponds to a registered builder in the `PipelineBuilder`.
 * New pipeline types (e.g., for Stable Diffusion, generic OpenCV filters) can
 * be added here and registered in the central factory.
 */
enum class PipelineType : std::uint8_t {
    /**
     * @brief Halide-based pipeline for fused pixel operations (Brightness, Contrast, etc.).
     */
    HalideOperation,

    // Future types can be added here:
    // SkyAI = 1,
    // FilterGeneric = 2
};

} // namespace Pipeline

} // namespace CaptureMoment::Core
