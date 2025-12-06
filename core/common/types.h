/**
 * @file types.h
 * @brief Entry point for all core type definitions and interfaces
 * @author CaptureMoment Team
 * @date 2025
 *
 * This file aggregates all the basic types of the project.
 * Include this file instead of including each header individually.
 *
 * @code
 * #include "common/types.h"
 * @endcode
 */

#pragma once

// ============================================================================
// Core Types
// ============================================================================
#include "pixel_format.h"
#include "image_region.h"
#include "operation_type.h"
#include "operation_descriptor.h"

// ============================================================================
// Core Interfaces
// ============================================================================
#include "i_operation.h"

// ============================================================================
// Core Components
// ============================================================================
#include "operation_factory.h"
#include "source_manager.h"
#include "image_pipeline.h"

/**
 * @namespace CaptureMoment
 * @brief Main namespace for the CaptureMoment project
 *
 * All classes, structures, and functions of the project are within
 * this namespace to avoid name conflicts.
 *
 * @par Core Types
 * - PixelFormat: Pixel format enumeration
 * - ImageRegion: Image tile structure with pixel data
 * - OperationType: Operation type enumeration
 * - OperationDescriptor: Operation configuration with parameters
 *
 * @par Core Interfaces
 * - IOperation: Abstract base for all image operations
 *
 * @par Core Components
 * - OperationFactory: Factory for creating operations
 * - SourceManager: Image source management with OIIO
 * - ImagePipeline: Pipeline orchestration
 */
namespace CaptureMoment {
    // Types and classes are defined in individual headers (see above)
}