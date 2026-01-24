/**
 * @file benchmarking_backend_decider.h
 * @brief Concrete implementation of IBackendDecider that chooses the backend based on performance benchmarks.
 *
 * This class implements a sophisticated benchmarking strategy to determine the optimal
 * hardware backend (CPU or a specific GPU API) for the application. It evaluates
 * available GPU APIs based on a priority list and selects the one that provides
 * the best performance relative to a CPU baseline.
 *
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include "image_processing/interfaces/i_backend_decider.h"
#include <chrono>
#include <memory>
#include <optional>
#include <array>

namespace CaptureMoment::Core {

namespace ImageProcessing {

/**
 * @class BenchmarkingBackendDecider
 * @brief Implements IBackendDecider using a runtime performance comparison strategy.
 *
 * @details
 * This decider executes a small, representative image processing pipeline
 * (a simple brightness/contrast adjustment) on different hardware targets.
 * It compares the execution time of the CPU backend against available GPU backends.
 *
 * The decision logic is as follows:
 * 1. **CPU Baseline**: Establishes a reference time by running the benchmark on the CPU.
 * 2. **GPU Availability**: Scans the Halide host target to detect which GPU features are
 *    compiled in (e.g., CUDA, Vulkan, Metal).
 * 3. **Priority Testing**: Tests available GPU backends in a strict priority order:
 *    - CUDA (NVIDIA - High Performance)
 *    - DirectX 12 (Windows Native)
 *    - Metal (macOS/iOS Native)
 *    - Vulkan (Cross-Platform)
 *    - OpenCL (Legacy Fallback)
 * 4. **Selection**: Selects the GPU backend only if it provides a significant speedup
 *    (defined by `k_gpu_advantage_threshold`) over the CPU baseline.
 *
 * @note
 * The benchmark incurs a startup cost (typically a few hundred milliseconds) to ensure
 * that the optimal hardware is used for the lifetime of the application.
 */
class BenchmarkingBackendDecider final : public IBackendDecider {
public:
    /**
     * @brief Virtual destructor.
     * @details Defaulted as no resources are managed by this class itself.
     */
    ~BenchmarkingBackendDecider() override = default;

    /**
     * @brief Decides the optimal memory type (CPU or GPU) by running benchmarks.
     *
     * @details
     * This method executes the full benchmarking sequence:
     * 1. Creates a dummy test image buffer.
     * 2. Runs a CPU benchmark.
     * 3. Iterates through available GPU APIs.
     * 4. Compares the best GPU time against the CPU time.
     *
     * @return Common::MemoryType::GPU_MEMORY if a GPU is significantly faster.
     * @return Common::MemoryType::CPU_RAM if no GPU is available, or if the CPU is faster.
     */
    [[nodiscard]] Common::MemoryType decide() const override;

private:
    // ============================================================
    // Configuration Constants
    // ============================================================

    /**
     * @brief Width of the test image used for benchmarking.
     * @details Set to 1920 (Full HD). This size is large enough to overcome
     * kernel launch overhead but small enough to keep the benchmark fast (< 1s).
     */
    static constexpr int k_benchmark_width = 1920;

    /**
     * @brief Height of the test image used for benchmarking.
     * @details Set to 1080 (Full HD).
     */
    static constexpr int k_benchmark_height = 1080;

    /**
     * @brief Number of color channels in the test image.
     * @details Set to 4 (RGBA). This represents the typical working format of the application.
     */
    static constexpr int k_benchmark_channels = 4;

    /**
     * @brief Performance threshold required to select the GPU backend.
     * @details Value is 0.9 (90%). The GPU must be at least 10% faster than the CPU
     * to be selected. This margin accounts for the overhead of data transfer
     * (Host -> Device) and potential driver overhead. If times are close, the CPU is preferred
     * for simplicity and power consumption.
     */
    static constexpr double k_gpu_advantage_threshold = 0.9;

    // ============================================================
    // Benchmarking Methods
    // ============================================================

    /**
     * @brief Benchmarks the CPU backend execution time.
     *
     * @details
     * This method allocates a test buffer, fills it with dummy data, creates a
     * simple Halide pipeline (arithmetic operations), and measures the time
     * taken to `realize` the output.
     *
     * @return std::chrono::milliseconds The duration of the CPU execution.
     * @return std::chrono::milliseconds::max() If an exception occurs during the benchmark.
     */
    [[nodiscard]] std::chrono::milliseconds benchmark_cpu() const;

    /**
     * @brief Benchmarks a specific GPU backend feature.
     *
     * @details
     * This method tests a specific Halide Target::Feature (e.g., CUDA, Vulkan).
     * It performs the following steps:
     * 1. Creates a Target object configured with the requested feature.
     * 2. Schedules the Halide pipeline for GPU execution (using tiling).
     * 3. Copies the test buffer to the device (Host -> Device).
     * 4. Executes the pipeline and measures the time.
     *
     * @param feature The Halide::Target::Feature to benchmark (e.g., Target::CUDA).
     * @param test_buffer A reference to the input buffer defining dimensions and data.
     * @return std::optional<std::chrono::milliseconds> The duration of the GPU execution if successful.
     * @return std::nullopt If the feature is unsupported, allocation fails, or copy_to_device fails.
     */
    [[nodiscard]] std::optional<std::chrono::milliseconds>
    benchmark_gpu_feature(Halide::Target::Feature feature, const Halide::Buffer<float>& test_buffer) const;
};

} // namespace ImageProcessing

} // namespace CaptureMoment::Core
