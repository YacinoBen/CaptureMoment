/**
 * @file workers.h
 * @brief Umbrella header for the Processing Workers module.
 *
 * @details
 * This module contains the low-level logic that executes specific **tasks** on image data.
 * Workers implement the `IWorkerRequest` interface, defining how a particular piece of
 * processing (e.g., Halide pipeline execution, OpenCV filter, AI inference) is run
 * asynchronously.
 *
 * **Architecture:**
 * - **Interface**: `IWorkerRequest` defines the contract for all workers (`execute(context, image)`).
 * - **Registry**: `WorkerBuilder` and `WorkerRegistry` manage the creation of concrete worker types.
 * - **Context**: `WorkerContext` owns the builder and provides access to specific worker instances.
 * - **Concrete**: `HalideOperationWorker` implements the execution logic for Halide-based adjustments.
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

// ============================================================
// 1. Worker Interface
// ============================================================

/**
 * @brief The abstract interface for all asynchronous processing workers.
 * Defines the contract `execute(context, image)`.
 */
#include "workers/interfaces/i_worker_request.h"

// ============================================================
// 2. Worker Infrastructure (The "Factory" & "Container")
// ============================================================

/**
 * @brief Central registry and factory for creating `IWorkerRequest` instances.
 * Allows registration of different worker types (Halide, OpenCV, AI) at startup.
 */
#include "workers/worker_builder.h"

/**
 * @brief Static helper to populate the `WorkerBuilder` registry.
 * Should be called once at application startup.
 */
#include "workers/worker_registry.h"

/**
 * @brief The central container managing the worker builder and active worker instances.
 * This is the primary class that the application (e.g., StateImageManager) should interact with
 * to retrieve specific worker objects.
 */
#include "workers/worker_context.h"

/**
 * @brief Enumeration defining supported worker types (Halide, OpenCV, AI, etc.).
 * Used as a key in the registry.
 */
#include "workers/worker_type.h"

// ============================================================
// 3. Concrete Workers
// ============================================================

/**
 * @brief Concrete worker for executing Halide-based adjustment operations.
 * Implements the `IWorkerRequest` interface for Halide pipelines.
 */
#include "workers/halide/halide_operation_worker.h"
