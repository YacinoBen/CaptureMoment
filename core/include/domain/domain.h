/**
 * @file domain.h
 * @brief Umbrella header for the Domain module.
 *
 * This header provides a single entry point for the `CaptureMoment::Core::Domain` namespace.
 * Including this file grants access to all interfaces defining the core abstractions
 * for image processing backends and tasks.
 *
 * This module is responsible for:
 * - **Abstraction**: Defining contracts for processing backends and tasks.
 * - **Extensibility**: Allowing new backends and task types to be integrated seamlessly.
 * - **Separation of Concerns**: Isolating business logic from implementation details.
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

// ============================================================
// 1. Interfaces
// ============================================================

/**
 * @brief Interface defining the contract for an image processing backend.
 * Allows for different processing implementations (CPU, GPU, AI-accelerated).
 */
#include "domain/i_processing_backend.h"

/**
 * @brief Interface defining the contract for an image processing task.
 * Encapsulates a unit of work to be performed on image data.
 */
#include "domain/i_processing_task.h"
