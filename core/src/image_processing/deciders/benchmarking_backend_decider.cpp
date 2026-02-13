/**
 * @file benchmarking_backend_decider.cpp
 * @brief Implementation of BenchmarkingBackendDecider using std::array for fixed GPU priority list.
 * @details Strict priority order enforced: CUDA > DX12 > Metal > Vulkan > OpenCL (Legacy).
 * @author CaptureMoment Team
 * @date 2026
 */

#include "image_processing/deciders/benchmarking_backend_decider.h"
#include <spdlog/spdlog.h>
#include <Halide.h>
#include <array>
#include <string>

namespace CaptureMoment::Core::ImageProcessing {

// Gets human-readable name for a Target::Feature
static std::string feature_to_string(Halide::Target::Feature feature)
{
    switch (feature)
    {
    case Halide::Target::CUDA: return "CUDA";
    case Halide::Target::OpenCL: return "OpenCL";
    case Halide::Target::Vulkan: return "Vulkan";
    case Halide::Target::Metal: return "Metal";
    case Halide::Target::D3D12Compute: return "DirectX12";
    default: return "Unknown";
    }
}

// Creates a simple pipeline (brightness adjustment) for benchmarking
static Halide::Func create_benchmark_pipeline(const Halide::Buffer<float>& input)
{
    Halide::Var x, y, c;
    Halide::Func pipeline;
    // Simple operation: Out = In * 1.1 + 0.05
    pipeline(x, y, c) = input(x, y, c) * 1.1f + 0.05f;
    return pipeline;
}

// ============================================================
// Main Decision Logic
// ============================================================

Common::MemoryType BenchmarkingBackendDecider::decide()
{
    spdlog::info("[BackendDecider] Starting backend performance benchmark...");

    // --- Phase 1: CPU Benchmark ---
    auto cpu_time = benchmark_cpu();

    if (cpu_time == std::chrono::milliseconds::max())
    {
        spdlog::warn("[BackendDecider] CPU benchmark failed completely. Defaulting to CPU.");
        m_winning_target = Halide::get_host_target();
        return Common::MemoryType::CPU_RAM;
    }
    spdlog::info("[BackendDecider] CPU Baseline: {} ms", cpu_time.count());

    // --- Phase 2: Setup Test Buffer for GPU ---
    // We create a single buffer definition. It will be copied to device during benchmark.
    Halide::Buffer<float> test_buffer(k_benchmark_width, k_benchmark_height, k_benchmark_channels);

    // Fill with dummy data (gradient)
    for (int c = 0; c < k_benchmark_channels; ++c)
    {
        for (int y = 0; y < k_benchmark_height; ++y)
        {
            for (int x = 0; x < k_benchmark_width; ++x)
            {
                test_buffer(x, y, c) = static_cast<float>((x + y + c) % 256) / 255.0f;
            }
        }
    }

    // Create pipeline graph once (it's the same for all backends)
    auto pipeline = create_benchmark_pipeline(test_buffer);

    // --- Phase 3: Detect & Benchmark GPUs (Strict Priority Order) ---

    // PRIORITY LOGIC:
    // 1. CUDA: NVIDIA Hardware-specific, highly optimized.
    // 2. DirectX 12: Windows Native, low overhead.
    // 3. Metal: macOS/iOS Native, best integration on Apple Silicon.
    // 4. Vulkan: Cross-platform, robust, modern.
    // 5. OpenCL: Legacy / Fallback. Less performant than Vulkan/DX12. Always last.

    std::array<std::pair<Halide::Target::Feature, std::string>, 5> gpu_priorities =
        { {
            {Halide::Target::CUDA, "CUDA"},              // High Priority (NVIDIA)
            {Halide::Target::D3D12Compute, "DirectX12"}, // High Priority (Windows)
            {Halide::Target::Metal, "Metal"},             // High Priority (macOS)
            {Halide::Target::Vulkan, "Vulkan"},           // Medium Priority (Cross-platform)
            {Halide::Target::OpenCL, "OpenCL"}            // Low Priority (Legacy / Fallback) - NEVER FIRST
        } };

    Halide::Target host_target = Halide::get_host_target();
    std::optional<std::chrono::milliseconds> best_gpu_time;
    std::string best_gpu_name = "None";

    for (const auto& [feature, name] : gpu_priorities)
    {
        if (!host_target.has_feature(feature))
        {
            spdlog::debug("[BackendDecider] {} not supported by Halide build.", name);
            continue;
        }

        auto result = benchmark_gpu_feature(feature, test_buffer);
        if (result.has_value())
        {
            if (!best_gpu_time.has_value() || result < best_gpu_time)
            {
                best_gpu_time = result;
                best_gpu_name = name;
                spdlog::info("[BackendDecider] {} benchmarked in {} ms (Current Best)", name, result.value().count());
            }
            else
            {
                spdlog::debug("[BackendDecider] {} benchmarked in {} ms (Slower than {})",
                              name, result.value().count(), best_gpu_name);
            }
        }
    }

    // --- Phase 4: Final Decision ---
    if (!best_gpu_time.has_value())
    {
        spdlog::info("[BackendDecider] No GPU benchmark succeeded. Using CPU backend.");
        m_winning_target = host_target; // Fallback to Host
        return Common::MemoryType::CPU_RAM;
    }

    spdlog::info("[BackendDecider] Best GPU: {} at {} ms", best_gpu_name, best_gpu_time.value().count());

    // Check threshold (GPU must be significantly faster)
    long long threshold_ms = static_cast<long long>(cpu_time.count() * k_gpu_advantage_threshold);

    if (best_gpu_time.value().count() < threshold_ms)
    {
        spdlog::info("[BackendDecider] GPU ({} ms) is significantly faster than CPU ({} ms). SELECTING GPU.",
                     best_gpu_time.value().count(), cpu_time.count());

        // Reconstruct the winning feature from the name to set it properly
        Halide::Target::Feature winning_feature = Halide::Target::OpenCL; // Default
        if (best_gpu_name == "CUDA") winning_feature = Halide::Target::CUDA;
        else if (best_gpu_name == "DirectX12") winning_feature = Halide::Target::D3D12Compute;
        else if (best_gpu_name == "Metal") winning_feature = Halide::Target::Metal;
        else if (best_gpu_name == "Vulkan") winning_feature = Halide::Target::Vulkan;

        m_winning_target = host_target;
        m_winning_target.set_feature(winning_feature);

        return Common::MemoryType::GPU_MEMORY;
    }
    else
    {
        spdlog::info("[BackendDecider] CPU ({} ms) is comparable or faster than GPU ({} ms). SELECTING CPU.",
                     cpu_time.count(), best_gpu_time.value().count());
        m_winning_target = host_target;
        return Common::MemoryType::CPU_RAM;
    }
}

// ============================================================
// Benchmark Implementations
// ============================================================

std::chrono::milliseconds BenchmarkingBackendDecider::benchmark_cpu() const
{
    try
    {
        Halide::Buffer<float> buffer(k_benchmark_width, k_benchmark_height, k_benchmark_channels);
        // Fill minimal data (value doesn't matter for compute-bound test, but prevents optimization away)
        buffer.fill(0.5f);

        auto pipeline = create_benchmark_pipeline(buffer);

        auto start = std::chrono::high_resolution_clock::now();
        // Realize explicitly
        pipeline.realize(buffer);
        auto end = std::chrono::high_resolution_clock::now();

        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    }
    catch (const std::exception& e)
    {
        spdlog::error("[BackendDecider] CPU Benchmark Exception: {}", e.what());
        return std::chrono::milliseconds::max();
    }
}

std::optional<std::chrono::milliseconds>
BenchmarkingBackendDecider::benchmark_gpu_feature(Halide::Target::Feature feature, const Halide::Buffer<float>& ref_buffer) const
{
    try
    {
        // Create target configuration
        Halide::Target target = Halide::get_host_target();
        target.set_feature(feature);

        // Create a working buffer copy (we will copy this to device)
        Halide::Buffer<float> work_buffer(ref_buffer);

        // Pipeline creation (Schedule for GPU)
        auto pipeline = create_benchmark_pipeline(work_buffer);
        Halide::Var x, y, c;
        Halide::Var xo, yo, xi, yi;
        // Standard GPU tiling strategy
        pipeline.gpu_tile(x, y, xo, yo, xi, yi, 16, 16);

        // 1. Copy to Device
        work_buffer.set_host_dirty();
        int copy_res = work_buffer.copy_to_device(target);
        if (copy_res != 0)
        {
            spdlog::debug("[BackendDecider] {} copy_to_device failed (err: {})", feature_to_string(feature), copy_res);
            return std::nullopt;
        }

        // 2. Benchmark Execution
        auto start = std::chrono::high_resolution_clock::now();
        // Realize on GPU
        Halide::Buffer<float> result = pipeline.realize({k_benchmark_width, k_benchmark_height, k_benchmark_channels});
        // Sync implicitly happens here or we rely on realize blocking
        auto end = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        spdlog::info("[BackendDecider] {} benchmark success: {} ms", feature_to_string(feature), duration.count());
        return duration;

    }
    catch (const std::exception& e)
    {
        spdlog::warn("[BackendDecider] {} Benchmark Exception: {}", feature_to_string(feature), e.what());
        return std::nullopt;
    }
}

} // namespace CaptureMoment::Core::ImageProcessing
