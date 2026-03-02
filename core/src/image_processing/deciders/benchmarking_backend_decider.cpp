/**
 * @file benchmarking_backend_decider.cpp
 * @brief Implementation of BenchmarkingBackendDecider
 * @details Strict priority order enforced: CUDA > DX12 > Metal > Vulkan > OpenCL (Legacy).
 * @author CaptureMoment Team
 * @date 2026
 */

#include "image_processing/deciders/benchmarking_backend_decider.h"
#include <spdlog/spdlog.h>
#include "Halide.h"
#include <array>
#include <string>

namespace CaptureMoment::Core::ImageProcessing {

// ============================================================
// Helper Functions
// ============================================================

/**
 * @brief Converts a Halide Target::Feature to a human-readable string.
 * @param feature The Halide feature enum value.
 * @return String representation of the feature name.
 */
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

/**
 * @brief Checks if a GPU feature should be tested on the current platform.
 * @param feature The GPU feature to check.
 * @return true if the feature should be tested, false to skip entirely.
 */
static bool should_test_feature(Halide::Target::Feature feature)
{
    switch (feature)
    {
    case Halide::Target::Metal:
#ifdef __APPLE__
        return true;
#else
        spdlog::debug("[BackendDecider] Metal skipped - not available on this platform");
        return false;
#endif

    case Halide::Target::D3D12Compute:
#ifdef _WIN32
        return true;
#else
        spdlog::debug("[BackendDecider] DirectX12 skipped - not available on this platform");
        return false;
#endif

    case Halide::Target::CUDA:
    case Halide::Target::OpenCL:
    case Halide::Target::Vulkan:
        return true;

    default:
        return false;
    }
}

/**
 * @brief Creates a fresh test buffer filled with gradient data.
 * @return A new Halide::Buffer<float> ready for GPU testing.
 *
 * @note IMPORTANT: Each GPU backend needs a FRESH buffer because:
 *       - Halide buffers become "tied" to a specific device interface
 *       - Reusing a buffer across different GPU backends causes:
 *         "halide_copy_to_device does not support switching interfaces"
 */
static Halide::Buffer<float> create_fresh_test_buffer(int width, int height, int channels)
{
    Halide::Buffer<float> buffer(width, height, channels);

    for (int c = 0; c < channels; ++c)
    {
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                buffer(x, y, c) = static_cast<float>((x + y + c) % 256) / 255.0f;
            }
        }
    }

    return buffer;
}

/**
 * @brief Creates a simple brightness adjustment pipeline for benchmarking.
 * @param input The input buffer to process.
 * @param x The x dimension variable.
 * @param y The y dimension variable.
 * @param c The c dimension variable.
 * @return A Halide Func representing the pipeline.
 */
static Halide::Func create_benchmark_pipeline(const Halide::Buffer<float>& input,
                                              Halide::Var& x, Halide::Var& y, Halide::Var& c)
{
    Halide::Func pipeline("benchmark_pipeline");
    pipeline(x, y, c) = input(x, y, c) * 1.1f + 0.05f;
    return pipeline;
}

/**
 * @brief Attempts to compile a pipeline for a GPU target.
 * @param pipeline The pipeline to compile.
 * @param target The target to compile for.
 * @return true if compilation succeeded, false otherwise.
 */
static bool try_compile_jit(Halide::Func& pipeline, const Halide::Target& target)
{
    try
    {
        pipeline.compile_jit(target);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

Common::MemoryType BenchmarkingBackendDecider::decide()
{
    spdlog::info("[BackendDecider] Starting backend performance benchmark...");

    // --- Phase 1: Log Host Target Info ---
    Halide::Target host_target = Halide::get_host_target();
    spdlog::info("[BackendDecider] Host target: {}", host_target.to_string());

    // --- Phase 2: CPU Benchmark ---
    auto cpu_time = benchmark_cpu();

    if (cpu_time == std::chrono::milliseconds::max())
    {
        spdlog::warn("[BackendDecider] CPU benchmark failed completely. Defaulting to CPU.");
        m_winning_target = host_target;
        return Common::MemoryType::CPU_RAM;
    }
    spdlog::info("[BackendDecider] CPU Baseline: {} ms", cpu_time.count());

    // --- Phase 3: Detect & Benchmark GPUs (Strict Priority Order) ---

    std::array<std::pair<Halide::Target::Feature, std::string>, 5> gpu_priorities =
        { {
            {Halide::Target::CUDA, "CUDA"},
            {Halide::Target::D3D12Compute, "DirectX12"},
            {Halide::Target::Metal, "Metal"},
            {Halide::Target::Vulkan, "Vulkan"},
            {Halide::Target::OpenCL, "OpenCL"}
        } };

    std::optional<std::chrono::milliseconds> best_gpu_time;
    std::string best_gpu_name = "None";
    Halide::Target::Feature best_gpu_feature = Halide::Target::OpenCL;

    for (const auto& [feature, name] : gpu_priorities)
    {
        // Skip backends that are not available on this platform
        if (!should_test_feature(feature))
        {
            continue;
        }

        spdlog::info("[BackendDecider] Testing {} backend...", name);

        // CRITICAL: Create a FRESH buffer for each backend test
        // Reusing buffers across different GPU interfaces causes:
        // "halide_copy_to_device does not support switching interfaces"
        Halide::Buffer<float> fresh_buffer = create_fresh_test_buffer(
            k_benchmark_width, k_benchmark_height, k_benchmark_channels);

        auto result = benchmark_gpu_feature(feature, fresh_buffer);

        if (result.has_value())
        {
            if (!best_gpu_time.has_value() || result < best_gpu_time)
            {
                best_gpu_time = result;
                best_gpu_name = name;
                best_gpu_feature = feature;
                spdlog::info("[BackendDecider] {} benchmarked in {} ms (Current Best)",
                             name, result.value().count());
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
        m_winning_target = host_target;
        return Common::MemoryType::CPU_RAM;
    }

    spdlog::info("[BackendDecider] Best GPU: {} at {} ms", best_gpu_name, best_gpu_time.value().count());

    long long threshold_ms = static_cast<long long>(cpu_time.count() * k_gpu_advantage_threshold);

    if (best_gpu_time.value().count() < threshold_ms)
    {
        spdlog::info("[BackendDecider] GPU ({} ms) is significantly faster than CPU ({} ms). SELECTING GPU.",
                     best_gpu_time.value().count(), cpu_time.count());

        m_winning_target = host_target;
        m_winning_target.set_feature(best_gpu_feature);

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

std::chrono::milliseconds BenchmarkingBackendDecider::benchmark_cpu() const
{
    try
    {
        Halide::Var x, y, c;
        Halide::Buffer<float> buffer = create_fresh_test_buffer(
            k_benchmark_width, k_benchmark_height, k_benchmark_channels);

        auto pipeline = create_benchmark_pipeline(buffer, x, y, c);

        auto start = std::chrono::high_resolution_clock::now();
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
BenchmarkingBackendDecider::benchmark_gpu_feature(Halide::Target::Feature feature,
                                                  const Halide::Buffer<float>& ref_buffer) const
{
    try
    {
        // Step 1: Create target with GPU feature
        Halide::Target target = Halide::get_host_target();
        target.set_feature(feature);

        // Step 2: Create pipeline
        // Note: ref_buffer is already fresh (created in decide() for each backend)
        Halide::Buffer<float> work_buffer(ref_buffer);
        Halide::Var x, y, c, xo, yo, xi, yi;
        auto pipeline = create_benchmark_pipeline(work_buffer, x, y, c);
        pipeline.gpu_tile(x, y, xo, yo, xi, yi, 16, 16);

        // Step 3: Try JIT compilation FIRST
        if (!try_compile_jit(pipeline, target))
        {
            spdlog::debug("[BackendDecider] {} - JIT compilation failed (not supported by Halide build)",
                          feature_to_string(feature));
            return std::nullopt;
        }

        spdlog::debug("[BackendDecider] {} - JIT compilation successful", feature_to_string(feature));

        // Step 4: Copy to device
        // The buffer is fresh, so no interface conflict can occur
        work_buffer.set_host_dirty();
        int copy_res = work_buffer.copy_to_device(target);
        if (copy_res != 0)
        {
            spdlog::debug("[BackendDecider] {} - copy_to_device failed (err: {}), no GPU device available",
                          feature_to_string(feature), copy_res);
            return std::nullopt;
        }

        // Step 5: Benchmark execution
        auto start = std::chrono::high_resolution_clock::now();

        Halide::Buffer<float> result;
        try
        {
            result = pipeline.realize(
                {k_benchmark_width, k_benchmark_height, k_benchmark_channels}, target);
        }
        catch (const Halide::Error& e)
        {
            spdlog::debug("[BackendDecider] {} - realize failed: {}", feature_to_string(feature), e.what());
            return std::nullopt;
        }

        auto end = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        spdlog::info("[BackendDecider] {} benchmark success: {} ms",
                     feature_to_string(feature), duration.count());
        return duration;
    }
    catch (const Halide::Error& e)
    {
        spdlog::debug("[BackendDecider] {} Halide error: {}", feature_to_string(feature), e.what());
        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        spdlog::debug("[BackendDecider] {} exception: {}", feature_to_string(feature), e.what());
        return std::nullopt;
    }
    catch (...)
    {
        spdlog::debug("[BackendDecider] {} - unknown exception prevented", feature_to_string(feature));
        return std::nullopt;
    }
}

} // namespace CaptureMoment::Core::ImageProcessing
