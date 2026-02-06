/**
 * @file managers.h
 * @brief Umbrella header for the Managers module.
 *
 * This header provides a single entry point for the `CaptureMoment::Core::Managers` namespace.
 * Including this file grants access to all interfaces and concrete implementations
 * related to image source management, state management, and operation orchestration.
 *
 * This module is responsible for:
 * - **Source Management**: Loading, caching, and accessing original image data.
 * - **State Management**: Maintaining the cumulative state of applied operations.
 * - **Orchestration**: Coordinating the flow of image processing tasks and updates.
 * - **Threading**: Managing asynchronous updates and concurrent access safely.
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

// ============================================================
// 1. Interfaces
// ============================================================

/**
 * @brief Interface for image source management (loading, tiles, metadata).
 */
#include "managers/i_source_manager.h"

// ============================================================
// 2. Concrete Implementations
// ============================================================

/**
 * @brief Concrete implementation for image source management using OpenImageIO.
 */
#include "managers/source_manager.h"

/**
 * @brief Concrete implementation for managing cumulative image processing state.
 */
#include "managers/state_image_manager.h"
