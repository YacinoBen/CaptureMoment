/**
 * @file operations.h
 * @brief Umbrella header for the Operations module of CaptureMoment Core.
 *
 * @details
 * This header provides centralized access to the entire image processing operation ecosystem.
 * By including this file, you gain access to:
 *
 * - **Interfaces**: `IOperation`, `IOperationFusionLogic`, `IOperationDefaultLogic`.
 * - **Core Types**: `OperationType`, `OperationDescriptor`, `OperationFactory`.
 * - **Infrastructure**: `OperationPipeline` (executor) and `OperationRegistry`.
 * - **Configuration**: `OperationRanges` (for validation and defaults).
 * - **Implementations**: All concrete adjustments (Blacks, Brightness, Contrast, Highlights, Shadows, Whites).
 *
 * This file is intended for use in Precompiled Headers (PCH) or for simplifying
 * includes in files that interact heavily with the operation pipeline.
 *
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

// ============================================================
// Interfaces & Core Types
// ============================================================

/**
 * @brief Abstract base class for all operations.
 */
#include "operations/interfaces/i_operation.h"

/**
 * @brief Interface for Halide pipeline fusion logic.
 */
#include "operations/interfaces/i_operation_fusion_logic.h"

/**
 * @brief Interface for CPU fallback logic.
 */
#include "operations/interfaces/i_operation_default_logic.h"

/**
 * @brief Enumeration of supported operation types.
 */
#include "operations/operation_type.h"

// ============================================================
// Infrastructure & Factory
// ============================================================

/**
 * @brief Factory for creating operation instances.
 */
#include "operations/operation_factory.h"

/**
 * @brief Registry for registering all available operations with the factory.
 */
#include "operations/operation_registry.h"

/**
 * @brief Defines parameter ranges and defaults for operations.
 */
#include "operations/operation_ranges.h"

/**
 * @brief Pipeline executor for applying a list of operations.
 */
#include "operations/operation_pipeline.h"

// ============================================================
// Concrete Operation Implementations
// ============================================================

/**
 * @brief Implementation of the 'Blacks' adjustment.
 */
#include "operations/basic_adjustment_operations/operation_blacks.h"

/**
 * @brief Implementation of the 'Brightness' adjustment.
 */
#include "operations/basic_adjustment_operations/operation_brightness.h"

/**
 * @brief Implementation of the 'Contrast' adjustment.
 */
#include "operations/basic_adjustment_operations/operation_contrast.h"

/**
 * @brief Implementation of the 'Highlights' adjustment.
 */
#include "operations/basic_adjustment_operations/operation_highlights.h"

/**
 * @brief Implementation of the 'Shadows' adjustment.
 */
#include "operations/basic_adjustment_operations/operation_shadows.h"

/**
 * @brief Implementation of the 'Whites' adjustment.
 */
#include "operations/basic_adjustment_operations/operation_whites.h"