/**
 * @file common.h
 * @brief Umbrella header for the Common module of the CaptureMoment Core Library.
 *
 * This header provides a single entry point for the `CaptureMoment::Core::Common` namespace.
 * Including this file grants access to all fundamental data structures, types,
 * and utilities defined within the Common module.
 *
 * This module is responsible for:
 * - **Basic Types:** Defining core enums and types (e.g., `MemoryType`).
 * - **Data Structures:** Providing core POD structures (e.g., `ImageRegion`, `PixelFormat`).
 * - **Error Handling:** Containing foundational error types (e.g., `CoreError`).
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

// ============================================================
// 1. Error Handling
// ============================================================

/**
 * @brief Central error handling system for CaptureMoment Core.
 */
#include "common/error_handling/core_error.h"

// ============================================================
// 2. Data Structures
// ============================================================

/**
 * @brief Fundamental data structure representing a rectangular region of image data.
 */
#include "common/image_region.h"

/**
 * @brief Definition of supported pixel formats (e.g., RGBA_U8, RGBA_F32).
 */
#include "common/pixel_format.h"

// ============================================================
// 3. Common Types
// ============================================================

/**
 * @brief Enumerations for memory types (CPU RAM, GPU MEMORY).
 */
#include "common/types/memory_type.h"

/**
 * @brief Common type definitions for image processing (dimensions, channels, coordinates).
 */
#include "common/types/image_types.h"

// Add other common headers as needed
