/**
 * @file pipeline.h
 * @brief Umbrella header for the Pipeline module.
 *
 * This header provides a single entry point for the `CaptureMoment::Core::Pipeline` namespace.
 * Including this file grants access to all interfaces and concrete implementations
 * related to pipeline execution, operation building, and backend management.
 *
 * This module is responsible for:
 * - **Building**: Combining multiple operations into a single, optimized computational pass (e.g., Halide fusion).
 * - **Execution**: Running the built pipeline on image data, potentially using optimized paths (Halide).
 * - **Fallback**: Providing a generic sequential execution path when optimized paths are unavailable.
 * - **Management**: Handling the lifecycle and caching of compiled pipelines for efficiency.
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

// ============================================================
// 1. Interfaces
// ============================================================

/**
 * @brief Abstract interface for executing a pre-compiled image processing pipeline.
 * Defines the contract for running a pipeline on an image.
 */
#include "pipeline/interfaces/i_pipeline_executor.h"

/**
 * @brief Specialized interface for Halide-optimized pipeline executors.
 * Provides a method to execute directly on a raw Halide::Buffer for maximum performance.
 */
#include "pipeline/interfaces/i_halide_pipeline_executor.h"

// ============================================================
// 2. Concrete Implementations
// ============================================================

/**
 * @brief Concrete implementation for executing fused adjustment pipelines using Halide.
 * Implements the optimized "Fast Path" for CPU and GPU backends.
 */
#include "pipeline/operation_pipeline_executor.h"

/**
 * @brief Concrete implementation for executing a pipeline sequentially as a fallback.
 * Provides a generic execution path when optimized pipelines are not suitable.
 */
#include "pipeline/fallback_pipeline_executor.h"

// ============================================================
// 3. Core Infrastructure (The "Factory")
// ============================================================

/**
 * @brief Central registry and factory for creating pipeline executors.
 * Allows registration of different backends (Halide, etc.).
 */
#include "pipeline/pipeline_builder.h"

/**
 * @brief Static helper to populate the PipelineBuilder registry.
 * Should be called once at application startup.
 */
#include "pipeline/pipeline_registry.h"

/**
 * @brief The central container managing the pipeline builder and active strategies.
 * This is the primary class that the application (e.g., StateImageManager) should interact with.
 */
#include "pipeline/pipeline_context.h"

// ============================================================
// 4. Common Types
// ============================================================

/**
 * @brief Enumeration defining supported pipeline types (Halide, OpenCV, AI, etc.).
 * Used as a key in the registry.
 */
#include "pipeline/pipeline_type.h"
