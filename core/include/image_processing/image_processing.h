/**
 * @file image_processing.h
 * @brief Umbrella header for the ImageProcessing module.
 * @author CaptureMoment Team
 * @date 2026
 *
 * This header provides a single include point for all concrete implementations
 * of the IWorkingImageHardware interface, including both CPU and GPU backends.
 * Including this file will pull in all necessary dependencies to use any
 * available processing backend.
 */

#pragma once

// --- CPU Implementations ---
#include "image_processing/cpu/working_image_cpu_default.h"
#include "image_processing/cpu/working_image_cpu_halide.h"

// --- GPU Implementations ---
#include "image_processing/gpu/working_image_gpu_halide.h"

// --- Deciders (Optional, but often used together) ---
#include "image_processing/deciders/benchmarking_backend_decider.h"
// #include "image_processing/deciders/hardware_detection_backend_decider.h"
// #include "image_processing/deciders/user_preference_backend_decider.h"

// --- Core Interfaces (For completeness) ---
#include "image_processing/interfaces/i_working_image_hardware.h"
#include "image_processing/interfaces/i_backend_decider.h"
