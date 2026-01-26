/**
 * @file image_processing.h
 * @brief Umbrella header for the ImageProcessing module.
 *
 * This header provides a single entry point for the `CaptureMoment::Core::ImageProcessing` namespace.
 * Including this file grants access to all interfaces and concrete implementations
 * related to image storage (CPU/GPU), backend decision, and memory management.
 *
 * This module is responsible for:
 * - **Abstraction**: Hiding the complexity of CPU vs GPU memory access.
 * - **Backends**: providing optimized CPU (Vector/Halide) and GPU (Halide) implementations.
 * - **Selection**: Deciding which backend to use (Benchmarking, Hardware Detection).
 * - **Factory**: Creating the appropriate working image instances based on configuration.
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

// ============================================================
// 1. Interfaces
// ============================================================

/**
 * @brief Main hardware abstraction interface.
 * Defines the contract for any image buffer (CPU or GPU).
 */
#include "image_processing/interfaces/i_working_image_hardware.h"

/**
 * @brief Specific interface for CPU-based image buffers.
 */
#include "image_processing/cpu/interfaces/i_working_image_cpu.h"

/**
 * @brief Specific interface for GPU-based image buffers.
 */
#include "image_processing/gpu/interfaces/i_working_image_gpu.h"

/**
 * @brief Interface for backend selection strategies.
 */
#include "image_processing/interfaces/i_backend_decider.h"

// ============================================================
// 2. Concrete Implementations
// ============================================================

// --- Base Classes ---
#include "image_processing/halide/working_image_halide.h"

// --- CPU Implementations ---
#include "image_processing/cpu/working_image_cpu_default.h"
#include "image_processing/cpu/working_image_cpu_halide.h"

// --- GPU Implementations ---
#include "image_processing/gpu/working_image_gpu_halide.h"

// ============================================================
// 3. Factories
// ============================================================

/**
 * @brief Factory for creating working images based on MemoryType.
 * Uses a Registry pattern to allow dynamic backend registration.
 */
#include "image_processing/factories/working_image_factory.h"

// ============================================================
// 4. Deciders
// ============================================================

/**
 * @brief Benchmark-based backend decider.
 * Runs performance tests to select the fastest available backend.
 */
#include "image_processing/deciders/benchmarking_backend_decider.h"
