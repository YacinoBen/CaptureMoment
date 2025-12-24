/**
 * @file types.h
 * @brief Entry point for all core type definitions, interfaces, and main components
 * @author CaptureMoment Team
 * @date 2025
 *
 * This file aggregates all the essential types, interfaces, and main components
 * of the core library. Include this file instead of including each header individually.
 *
 * @code
 * #include "common/types.h"
 * @endcode
 */

#pragma once

// ============================================================================
// Core Types
// ============================================================================
#include "common/pixel_format.h"
#include "common/image_region.h"
#include "operations/operation_type.h"
#include "operations/operation_descriptor.h"

// ============================================================================
// Core Interfaces
// ============================================================================
#include "operations/i_operation.h"
#include "managers/i_source_manager.h"
#include "domain/i_processing_task.h"
#include "domain/i_processing_backend.h"

// ============================================================================
// Core Components
// ============================================================================
#include "operations/operation_factory.h"
#include "managers/source_manager.h"      
#include "operations/operation_pipeline.h"
#include "engine/photo_task.h"             
#include "engine/photo_engine.h"

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
 * - ISourceManager: Abstract interface for image source management (load, getTile, setTile)
 * - IProcessingTask: Abstract interface for a unit of image processing work
 * - IProcessingBackend: Abstract interface for managing processing tasks
 *
 * @par Core Components
 * - OperationFactory: Factory for creating operations
 * - SourceManager: Concrete image source management with OIIO
 * - OperationPipeline: Stateless utility to apply a sequence of operations to an ImageRegion
 * - PhotoTask: Concrete implementation of IProcessingTask
 * - PhotoEngine: Concrete implementation of IProcessingBackend, orchestrates the processing flow
 */
namespace CaptureMoment::Core::Common {
    // Types and classes are defined in individual headers (see above)
}
