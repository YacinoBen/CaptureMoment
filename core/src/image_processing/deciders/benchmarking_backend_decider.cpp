/**
 * @file benchmarking_backend_decider.cpp
 * @brief Implementation of BenchmarkingBackendDecider with detailed GPU detection and a prioritized backend testing strategy.
 *
 * The priority order for backends is as follows:
 * 1. Hardware-Specific: CUDA (for NVIDIA GPUs).
 * 2. OS-Specific: DirectX12 (Windows), Metal (macOS).
 * 3. Cross-Platform: Vulkan (Windows, Linux, Android).
 * 4. Fallback/Legacy: OpenCL (widely supported but often slower).
 *
 * This ensures we use the most efficient and well-supported API for the current platform and hardware.
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#include "image_processing/deciders/benchmarking_backend_decider.h"
#include <spdlog/spdlog.h>
#include <Halide.h>
#include <vector>
#include <string>

namespace CaptureMoment::Core::ImageProcessing {

// A simple, representative Halide pipeline for benchmarking.
static Halide::Func create_benchmark_pipeline(const Halide::Buffer<float>& input)
{
    Halide::Var x, y, c;
    Halide::Func pipeline;
    pipeline(x, y, c) = input(x, y, c) * 1.1f + 0.05f;
    return pipeline;
}

// Helper function to get a human-readable name for a DeviceAPI
static std::string device_api_to_string(Halide::DeviceAPI api)
{
    switch (api) {
    case Halide::DeviceAPI::CUDA: return "CUDA";
    case Halide::DeviceAPI::OpenCL: return "OpenCL";
    case Halide::DeviceAPI::Vulkan: return "Vulkan";
    case Halide::DeviceAPI::Metal: return "Metal";
    case Halide::DeviceAPI::D3D12Compute: return "DirectX12";
    default: return "Unknown";
    }
}

// Helper function to get a human-readable name for a Target feature
static std::string target_feature_to_string(Halide::Target::Feature feature) {
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

// Structure to hold benchmark results
struct BackendResult
{
    Halide::DeviceAPI api;
    std::chrono::milliseconds time;
    bool is_valid;
};

// Performs a benchmark for a specific DeviceAPI
static BackendResult benchmark_for_device_api(
    Halide::DeviceAPI api,
    const Halide::Buffer<float>& test_buffer,
    const Halide::Func& pipeline
    )
{
    const int width = test_buffer.width();
    const int height = test_buffer.height();
    const int channels = test_buffer.channels();

    try {
        spdlog::debug("BenchmarkingBackendDecider: Testing {} backend...", device_api_to_string(api));

        // Create a target with the specific feature
        Halide::Target target = Halide::get_host_target();
        switch (api) {
        case Halide::DeviceAPI::CUDA:
            target.set_feature(Halide::Target::CUDA);
            break;
        case Halide::DeviceAPI::OpenCL:
            target.set_feature(Halide::Target::OpenCL);
            break;
        case Halide::DeviceAPI::Vulkan:
            target.set_feature(Halide::Target::Vulkan);
            break;
        case Halide::DeviceAPI::Metal:
            target.set_feature(Halide::Target::Metal);
            break;
        case Halide::DeviceAPI::D3D12Compute:
            target.set_feature(Halide::Target::D3D12Compute);
            break;
        default:
            spdlog::warn("BenchmarkingBackendDecider: Unsupported DeviceAPI requested.");
            return {api, std::chrono::milliseconds::max(), false};
        }

        // Copy buffer to device
        auto working_buffer = test_buffer;
        working_buffer.set_host_dirty();
        int copy_result = working_buffer.copy_to_device(target);
        if (copy_result != 0) {
            spdlog::warn("BenchmarkingBackendDecider: copy_to_device failed for {} with error code: {}",
                         device_api_to_string(api), copy_result);
            return {api, std::chrono::milliseconds::max(), false};
        }

        // Schedule and run
        Halide::Var x, y, c;
        Halide::Var xo, yo, xi, yi;
        auto scheduled_pipeline = pipeline;
        scheduled_pipeline.gpu_tile(x, y, xo, yo, xi, yi, 16, 16);

        auto start = std::chrono::high_resolution_clock::now();
        Halide::Buffer<float> output = scheduled_pipeline.realize({width, height, channels});
        output.copy_to_host(); // Synchronize
        auto end = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        spdlog::info("BenchmarkingBackendDecider: {} benchmark completed in {} ms",
                     device_api_to_string(api), duration.count());

        return {api, duration, true};

    } catch (const std::exception& e) {
        spdlog::warn("BenchmarkingBackendDecider: Exception during {} benchmark: {}",
                     device_api_to_string(api), e.what());
        return {api, std::chrono::milliseconds::max(), false};
    }
}

Common::MemoryType BenchmarkingBackendDecider::decide() const
{
    spdlog::info("BenchmarkingBackendDecider::decide Starting backend performance benchmark...");

    // --- LOG 1: Host target features ---
    Halide::Target host_target = Halide::get_host_target();

    spdlog::info("BenchmarkingBackendDecider::decide List Target Features:");

    std::vector<std::pair<Halide::Target::Feature, std::string>> features_to_check = {
        {Halide::Target::CUDA, "CUDA"},
        {Halide::Target::OpenCL, "OpenCL"},
        {Halide::Target::Vulkan, "Vulkan"},
        {Halide::Target::Metal, "Metal"},
        {Halide::Target::D3D12Compute, "DirectX12"}
    };

    for (const auto& [feature, name] : features_to_check)
    {
        if (host_target.has_feature(feature)) {
            spdlog::info("  - {}: Supported", name);
        } else {
            spdlog::info("  - {}: Not Supported", name);
        }
    }

    spdlog::info("BenchmarkingBackendDecider::decide Try toEnable Host Target Features:");
    std::vector<Halide::Target::Feature> supported_features;
    if (host_target.has_feature(Halide::Target::CUDA)) {
        spdlog::info("BenchmarkingBackendDecider::decide - CUDA: Supported");
        supported_features.push_back(Halide::Target::CUDA);
    }
    if (host_target.has_feature(Halide::Target::OpenCL)) {
        spdlog::info("BenchmarkingBackendDecider::decide - OpenCL: Supported");
        supported_features.push_back(Halide::Target::OpenCL);
    }
    if (host_target.has_feature(Halide::Target::Vulkan)) {
        spdlog::info("BenchmarkingBackendDecider::decide - Vulkan: Supported");
        supported_features.push_back(Halide::Target::Vulkan);
    }
    if (host_target.has_feature(Halide::Target::Metal)) {
        spdlog::info("BenchmarkingBackendDecider::decide - Metal: Supported");
        supported_features.push_back(Halide::Target::Metal);
    }
    if (host_target.has_feature(Halide::Target::D3D12Compute)) {
        spdlog::info("BenchmarkingBackendDecider::decide - DirectX12: Supported");
        supported_features.push_back(Halide::Target::D3D12Compute);
    }
    if (supported_features.empty()) {
        spdlog::warn("BenchmarkingBackendDecider: No GPU features detected. Falling back to CPU.");
        return Common::MemoryType::CPU_RAM;
    }

    // --- LOG 2: Probe available devices (best-effort) ---
    for (auto feature : supported_features)
    {
        try {
            Halide::Target target = Halide::get_host_target();
            target.set_feature(feature);
            Halide::Buffer<float> probe(1, 1, 1);
            probe.set_host_dirty();
            int result = probe.copy_to_device(target);
            if (result == 0) {
                spdlog::info("BenchmarkingBackendDecider: Successfully initialized {} device.",
                             target_feature_to_string(feature));
            } else {
                spdlog::warn("BenchmarkingBackendDecider: Failed to initialize {} device (error {}).",
                             target_feature_to_string(feature), result);
            }
        } catch (...) {
            spdlog::warn("BenchmarkingBackendDecider: Failed to probe {} device.",
                         target_feature_to_string(feature));
        }
    }

    // --- STEP 1: Benchmark CPU ---
    auto cpu_time = benchmarkCPU();
    spdlog::info("BenchmarkingBackendDecider: CPU benchmark completed in {} ms", cpu_time.count());

    // --- STEP 2: Create a shared test buffer ---
    const int width = 1920;
    const int height = 1080;
    const int channels = 4;
    Halide::Buffer<float> test_buffer(width, height, channels);

    for (int c = 0; c < channels; ++c) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                test_buffer(x, y, c) = static_cast<float>((x + y + c) % 256) / 255.0f;
            }
        }
    }
    auto pipeline = create_benchmark_pipeline(test_buffer);

    // --- STEP 3: Test GPU backends in priority order ---
    // Priority Order:
    // 1. Hardware-Specific (NVIDIA)
    // 2. OS-Specific (Windows, macOS)
    // 3. Cross-Platform (Vulkan)
    // 4. Fallback/Legacy (OpenCL)
    std::vector<Halide::DeviceAPI> priority_order = {
        Halide::DeviceAPI::CUDA,        // 1. Hardware-specific for NVIDIA
        Halide::DeviceAPI::D3D12Compute, // 2. OS-specific for Windows
        Halide::DeviceAPI::Metal,        // 2. OS-specific for macOS
        Halide::DeviceAPI::Vulkan,       // 3. Cross-platform (Windows, Linux, Android)
        Halide::DeviceAPI::OpenCL        // 4. Fallback/Legacy
    };

    BackendResult best_gpu_result = {Halide::DeviceAPI::None, std::chrono::milliseconds::max(), false};

    for (auto api : priority_order)
    {
        // Check if the backend is supported by the current Halide build
        bool is_supported = false;
        switch (api) {
        case Halide::DeviceAPI::CUDA:
            is_supported = host_target.has_feature(Halide::Target::CUDA);
            break;
        case Halide::DeviceAPI::D3D12Compute:
            is_supported = host_target.has_feature(Halide::Target::D3D12Compute);
            break;
        case Halide::DeviceAPI::Metal:
            is_supported = host_target.has_feature(Halide::Target::Metal);
            break;
        case Halide::DeviceAPI::Vulkan:
            is_supported = host_target.has_feature(Halide::Target::Vulkan);
            break;
        case Halide::DeviceAPI::OpenCL:
            is_supported = host_target.has_feature(Halide::Target::OpenCL);
            break;
        default:
            continue;
        }

        if (!is_supported) {
            spdlog::debug("BenchmarkingBackendDecider: Skipping {} (not supported in this Halide build).", device_api_to_string(api));
            continue;
        }

        auto result = benchmark_for_device_api(api, test_buffer, pipeline);
        if (result.is_valid && result.time < best_gpu_result.time) {
            best_gpu_result = result;
        }
    }

    // --- STEP 4: Make the final decision ---
    spdlog::info("BenchmarkingBackendDecider: Best GPU time: {} ms (using {})",
                 best_gpu_result.time.count(), device_api_to_string(best_gpu_result.api));

    // Add a small margin (e.g., 10%) to favor CPU if times are very close,
    // to avoid the overhead of GPU memory transfers for marginal gains.
    const double gpu_advantage_threshold = 0.9;

    if (best_gpu_result.is_valid &&
        best_gpu_result.time.count() < static_cast<long long>(cpu_time.count() * gpu_advantage_threshold)) {
        spdlog::info("BenchmarkingBackendDecider: GPU ({}) is faster. Selecting GPU backend.",
                     device_api_to_string(best_gpu_result.api));
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
        Halide::Buffer<float> test_buffer(width, height, channels);
        for (int c = 0; c < channels; ++c) {
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    test_buffer(x, y, c) = static_cast<float>((x + y + c) % 256) / 255.0f;
                }
            }
        }

        auto pipeline = create_benchmark_pipeline(test_buffer);
        auto start = std::chrono::high_resolution_clock::now();
        Halide::Buffer<float> output = pipeline.realize({width, height, channels});
        auto end = std::chrono::high_resolution_clock::now();

        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    } catch (const std::exception& e) {
        spdlog::warn("BenchmarkingBackendDecider::benchmarkCPU: Exception occurred: {}", e.what());
        return std::chrono::milliseconds::max();
    }
}

} // namespace CaptureMoment::Core::ImageProcessing
