/**
 * @file benchmarking_backend_decider.cpp
 * @brief Implementation of BenchmarkingBackendDecider
 * @author CaptureMoment Team
 * @date 2026
 */

#include "image_processing/deciders/benchmarking_backend_decider.h"
#include <spdlog/spdlog.h>
#include <Halide.h>
#include <algorithm>

// A simple, representative Halide pipeline for benchmarking.
// This function defines a basic brightness/contrast-like operation.
namespace CaptureMoment::Core::ImageProcessing {

static Halide::Func create_benchmark_pipeline(const Halide::Buffer<float>& input)
{
    Halide::Var x, y, c;
    Halide::Func pipeline;
    // Simple linear transform: output = input * 1.1f + 0.05f
    pipeline(x, y, c) = input(x, y, c) * 1.1f + 0.05f;
    return pipeline;
}

Common::MemoryType BenchmarkingBackendDecider::decide() const {
    spdlog::info("BenchmarkingBackendDecider: Starting backend performance benchmark...");

    auto cpu_time = benchmarkCPU();
    auto gpu_time = benchmarkGPU();

    spdlog::info("BenchmarkingBackendDecider: CPU time: {} ms, GPU time: {} ms",
                 cpu_time.count(), gpu_time.count());

    // Add a small margin (e.g., 10%) to favor CPU if times are very close,
    // to avoid the overhead of GPU memory transfers for marginal gains.
    const double gpu_advantage_threshold = 0.9; // GPU must be at least 10% faster

    if (gpu_time.count() < static_cast<long long>(cpu_time.count() * gpu_advantage_threshold)) {
        spdlog::info("BenchmarkingBackendDecider: GPU is faster. Selecting GPU backend.");
        return Common::MemoryType::GPU_MEMORY;
    } else {
        spdlog::info("BenchmarkingBackendDecider: CPU is faster or comparable. Selecting CPU backend.");
        return Common::MemoryType::CPU_RAM;
    }
}

std::chrono::milliseconds BenchmarkingBackendDecider::benchmarkCPU() const
{
    const int width = 1920;
    const int height = 1080;
    const int channels = 4;

    try {
        // Create a test buffer on CPU
        Halide::Buffer<float> test_buffer(width, height, channels);
        // Initialize with deterministic data
        for (int c = 0; c < channels; ++c) {
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    test_buffer(x, y, c) = static_cast<float>((x + y + c) % 256) / 255.0f;
                }
            }
        }

        auto pipeline = create_benchmark_pipeline(test_buffer);
        // Schedule for CPU (default)
        // No special scheduling needed; Halide will use CPU by default.

        auto start = std::chrono::high_resolution_clock::now();
        Halide::Buffer<float> output = pipeline.realize({width, height, channels});
        auto end = std::chrono::high_resolution_clock::now();

        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    } catch (const std::exception& e) {
        spdlog::warn("BenchmarkingBackendDecider::benchmarkCPU: Exception occurred: {}", e.what());
        return std::chrono::milliseconds::max();
    }
}

std::chrono::milliseconds BenchmarkingBackendDecider::benchmarkGPU() const {
    Halide::Target target = Halide::get_host_target();
    if (!target.has_gpu_feature()) {
        spdlog::debug("BenchmarkingBackendDecider::benchmarkGPU: No GPU feature detected in host target.");
        return std::chrono::milliseconds::max();
    }

    const int width = 1920;
    const int height = 1080;
    const int channels = 4;

    try {
        // Create a test buffer on CPU first
        Halide::Buffer<float> test_buffer(width, height, channels);
        for (int c = 0; c < channels; ++c) {
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    test_buffer(x, y, c) = static_cast<float>((x + y + c) % 256) / 255.0f;
                }
            }
        }

        // Transfer to GPU
        test_buffer.set_host_dirty();
        int copy_result = test_buffer.copy_to_device(target);
        if (copy_result != 0) {
            spdlog::warn("BenchmarkingBackendDecider::benchmarkGPU: copy_to_device failed with error code: {}", copy_result);
            return std::chrono::milliseconds::max();
        }

        auto pipeline = create_benchmark_pipeline(test_buffer);
        
        // Declare the Vars used in the Func
        Halide::Var x, y, c;
        // Declare new Vars for tiling
        Halide::Var xo, yo, xi, yi;
        // Apply the GPU schedule
        pipeline.gpu_tile(x, y, xo, yo, xi, yi, 16, 16);

        auto start = std::chrono::high_resolution_clock::now();
        Halide::Buffer<float> output = pipeline.realize({width, height, channels});
        // Synchronize to ensure GPU computation is complete
        output.copy_to_host();
        auto end = std::chrono::high_resolution_clock::now();

        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    } catch (const std::exception& e) {
        spdlog::warn("BenchmarkingBackendDecider::benchmarkGPU: Exception occurred: {}", e.what());
        return std::chrono::milliseconds::max();
    }
}

} // namespace CaptureMoment::Core::ImageProcessing
