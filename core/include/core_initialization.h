/**
 * @file core_initialization.h
 * @brief Core library initialization entry point.
 *
 * This header provides the single public function required to initialize the
 * CaptureMoment Core library (Backends Registration, Benchmarking).
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

namespace CaptureMoment::Core {

/**
 * @brief Initializes the CaptureMoment Core library.
 *
 * @details
 * This function handles the complete startup sequence for the image processing core.
 * It uses a thread-safe `std::call_once` mechanism to ensure that
 * the heavy initialization logic (registration + benchmarking) is executed
 * exactly once during the application's lifetime.
 *
 * **Initialization Sequence:**
 * 1. Registers CPU/GPU backends into `WorkingImageFactory` (via `ImageProcessing::registerDefaultBackends`).
 * 2. Runs `BenchmarkingBackendDecider` to select optimal hardware (CPU vs GPU).
 * 3. Stores the selected backend in `AppConfig`.
 *
 *
 * @note This function is thread-safe.
 */
void initialize();

} // namespace CaptureMoment::Core
