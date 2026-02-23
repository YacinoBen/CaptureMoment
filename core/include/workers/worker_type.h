/**
 * @file worker_type.h
 * @brief Enumeration defining supported worker types.
 *
 * @details
 * This file contains the `WorkerType` enum which serves as a key
 * for the `WorkerContext` registry. It allows the system to identify
 * and instantiate the correct concrete processing task (e.g., Halide, OpenCV, AI)
 * at runtime without hard dependencies.
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include <cstdint>

namespace CaptureMoment::Core {

namespace Workers {

/**
 * @enum WorkerType
 * @brief Identifies the specific technology or strategy used for a processing worker.
 *
 * @details
 * Each value corresponds to a registered builder in the `WorkerContext`.
 * New worker types (e.g., for Stable Diffusion, generic OpenCV filters) can
 * be added here and registered in the central factory.
 */
enum class WorkerType : std::uint8_t {
    /**
     * @brief Worker for Halide-based pixel operations (Brightness, Contrast, etc.).
     */
    HalideOperation = 0,
};

} // namespace Workers

} // namespace CaptureMoment::Core