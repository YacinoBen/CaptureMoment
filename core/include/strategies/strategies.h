/**
 * @file strategies.h
 * @brief Umbrella header for the Processing Strategies module.
 *
 * @details
 * This module contains the high-level logic that dictates **how** an image is processed.
 * Strategies act as wrappers around low-level executors, handling specific
 * domain logic (e.g., "Halide Adjustments", "Sky Replacement", "Color Grading").
 *
 * **Architecture:**
 * - **Interface**: `IPipelineManager` defines the contract for all strategies.
 * - **Concrete**: `PipelineHalideOperationManager` implements adjustments using Halide.
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

// ============================================================
// 1. Strategy Interface
// ============================================================

/**
 * @brief The abstract interface for all processing strategies.
 * Defines the contract `init(ops)` and `execute(image)`.
 */
#include "strategies/pipeline/interfaces/i_pipeline_manager.h"

// ============================================================
// 2. Concrete Strategies
// ============================================================

/**
 * @brief Concrete strategy for fused Halide adjustments (Brightness, Contrast, etc.).
 * Manages the lifecycle of a `OperationPipelineExecutor` and handles configuration updates.
 */
#include "strategies/pipeline/pipeline_halide_operation_manager.h"