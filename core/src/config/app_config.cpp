/**
 * @file app_config.cpp
 * @brief Implementation of AppConfig.
 * @author CaptureMoment Team
 * @date 2026
 */

#include "config/app_config.h"
#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::Config {

// ============================================================
// Static Member Initialization
// ============================================================


Halide::Target AppConfig::s_halide_target = Halide::get_host_target();

// ============================================================
// Singleton Instance
// ============================================================

AppConfig& AppConfig::instance()
{
    // C++11 Magic Static: Thread-safe initialization.
    static AppConfig s_instance;
    return s_instance;
}

// ============================================================
// Backend Configuration
// ============================================================

void AppConfig::setProcessingBackend(Common::MemoryType backend)
{
    if (m_processing_backend != backend)
    {
        m_processing_backend = backend;

        const auto backend_str = (backend == Common::MemoryType::CPU_RAM)
                                     ? "CPU_RAM"
                                     : "GPU_MEMORY";

        spdlog::info("[AppConfig] Processing backend changed to: {}", backend_str);
    }
}

Common::MemoryType AppConfig::getProcessingBackend() const noexcept
{
    return m_processing_backend;
}

// ============================================================
// Halide Target Configuration
// ============================================================

void AppConfig::setHalideTarget(const Halide::Target& target)
{
    // Update the global static target
    s_halide_target = target;

    // Log the specific features configured in this target (helpful for debugging)
    std::string features_str;
    if (target.has_feature(Halide::Target::CUDA)) features_str += "CUDA ";
    if (target.has_feature(Halide::Target::OpenCL)) features_str += "OpenCL ";
    if (target.has_feature(Halide::Target::Vulkan)) features_str += "Vulkan ";
    if (target.has_feature(Halide::Target::Metal)) features_str += "Metal ";
    if (target.has_feature(Halide::Target::D3D12Compute)) features_str += "DirectX12 ";

    spdlog::info("[AppConfig] Halide Target updated. Architecture: {}, Features: [{}]",
                 target.to_string(), features_str);
}

const Halide::Target& AppConfig::getHalideTarget()
{
    return s_halide_target;
}

// ============================================================
// Testing Utilities
// ============================================================

#ifdef ENABLE_TESTS
void AppConfig::reset()
{
    spdlog::debug("[AppConfig] Resetting configuration to defaults.");

    // Reset backend to default
    m_processing_backend = Common::MemoryType::CPU_RAM;

    // Reset target to default host (removes GPU features)
    s_halide_target = Halide::get_host_target();
}
#endif

} // namespace CaptureMoment::Core::Config
