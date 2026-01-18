/**
 * @file benchmarking_backend_decider.h
 * @brief Concrete implementation of IBackendDecider that chooses the backend based on a performance benchmark.
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "image_processing/interfaces/i_backend_decider.h"
#include <chrono>

namespace CaptureMoment::Core {

namespace ImageProcessing {

/**
 * @brief Concrete implementation of IBackendDecider that chooses the backend based on a performance benchmark.
 *
 * This decider runs a small, representative image processing operation (e.g., a brightness adjustment)
 * on both CPU and GPU backends and selects the faster one. It provides an optimal choice based on
 * actual hardware performance but incurs a startup cost for the benchmark itself.
 */
class BenchmarkingBackendDecider final : public IBackendDecider {
public:
    /**
     * @brief Virtual destructor.
     */
    ~BenchmarkingBackendDecider() override = default;

    /**
     * @brief Decides the memory type (CPU or GPU) by running a performance benchmark.
     *
     * This method creates a test image buffer, runs a simple Halide pipeline on both
     * CPU and GPU (if available), measures the execution time, and returns the backend
     * that performed faster.
     *
     * @return MemoryType::GPU_MEMORY if GPU is faster, otherwise MemoryType::CPU_RAM.
     */
    [[nodiscard]] Common::MemoryType decide() const override;

private:
    /**
     * @brief Benchmarks the CPU backend by running a simple Halide pipeline.
     * @return The execution time in milliseconds.
     */
    [[nodiscard]] std::chrono::milliseconds benchmarkCPU() const;

    /**
     * @brief Benchmarks the GPU backend by running a simple Halide pipeline.
     * @return The execution time in milliseconds, or max() if GPU is not available or fails.
     */
    [[nodiscard]] std::chrono::milliseconds benchmarkGPU() const;
};

} // namespace ImageProcessing

} // namespace CaptureMoment::Core
