/**
 * @file core_initialization.cpp
 * @brief Implementation of Core library initialization.
 * @details Orchestrates backend registration and benchmarking.
 * @author CaptureMoment Team
 * @date 2026
 */

#include "core_initialization.h"
#include "image_processing/registration/working_image_registration.h"
#include "image_processing/deciders/benchmarking_backend_decider.h"
#include "config/app_config.h"
#include <spdlog/spdlog.h>
#include <mutex>

namespace CaptureMoment::Core {

namespace {
    // Global flag used by std::call_once to ensure initialization runs only once.
    inline std::once_flag s_init_flag;

    // ============================================================
    // Helper: Backends Registration
    // ============================================================
    void init_backends()
    {
        // Call the registration function located in ImageProcessing namespace
        // This function registers CPU_Halide and GPU_Halide factories.
        CaptureMoment::Core::ImageProcessing::registerDefaultBackends();
    }

    // ============================================================
    // Helper: Backend Decider & Configuration
    // ============================================================
    void init_backend_decider()
    {
        spdlog::info("[CoreInitialization] Starting backend selection benchmark...");

        // 1. Create the decider
        CaptureMoment::Core::ImageProcessing::BenchmarkingBackendDecider benchmark_decider;

        // 2. Run benchmark (CPU vs GPU comparison)
        auto backend = benchmark_decider.decide();

        // 3. Store the selected MemoryType in global Config
        CaptureMoment::Core::Config::AppConfig::instance().setProcessingBackend(backend);

        // 4. Store the specific Halide Target (Host+CUDA, Host+Vulkan, etc.) in Config
        // This allows WorkingImageGPU_Halide to retrieve the correct target.
        CaptureMoment::Core::Config::AppConfig::setHalideTarget(benchmark_decider.getWinningTarget());

        spdlog::info("[CoreInitialization] Backend configuration complete.");
    }

    // ============================================================
    // Main Initialization Logic
    // ============================================================
    void perform_initialization()
    {
        // Step 1: Register Factories
        init_backends();

        // Step 2: Run Benchmark Decider & Setup Config
        init_backend_decider();
    }

} 

void initialize()
{
    // std::call_once ensures that 'perform_initialization' is executed exactly once.
    // It is thread-safe: if multiple threads call initialize(), they will synchronize,
    // and only one will execute the registration and benchmarking.
    std::call_once(s_init_flag, perform_initialization);
}

} // namespace CaptureMoment::Core
