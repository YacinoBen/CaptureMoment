/**
 * @file engine.h
 * @brief Umbrella header for the Engine module.
 *
 * This header provides a single entry point for the `CaptureMoment::Core::Engine` namespace.

 *
 * This module is responsible for:
 * - **Orchestration**: Central coordination between UI and Core (PhotoEngine).
 * - **UI Integration**: Providing easy access to the current working image for display.
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

// ============================================================
// 1. Concrete Implementations
// ============================================================

/**
 * @brief Central engine orchestrating image loading and cumulative operation management.
 * Acts as the main interface between the UI layer (Qt) and the core image processing logic.
 */
#include "engine/photo_engine.h"
