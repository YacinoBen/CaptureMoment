/**
 * @file working_image_registration.h
 * @brief Declaration of backend registration function.
 *
 * This file separates the declaration of registration logic from the factory implementation
 * and includes concrete backend headers only in the .cpp file to avoid circular dependencies.
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include <utility>

namespace CaptureMoment::Core {

namespace ImageProcessing {

/**
 * @brief Registers all default Core backends (CPU and GPU) to the factory.
 *
 * @details
 * This function must be called once during application startup (before any images
 * are processed) to populate the factory's registry. It includes the concrete
 * implementations (WorkingImageCPU_Halide, WorkingImageGPU_Halide) and registers them.
 *
 * If you add custom backends (e.g., Plugins), you can call this function
 * first and then register your own custom backends.
 *
 * @note This function is NOT thread-safe regarding the underlying factory registry map.
 *       Ensure it is called from a single thread at startup.
 */
void registerDefaultBackends();

} // namespace ImageProcessing

} // namespace CaptureMoment::Core
