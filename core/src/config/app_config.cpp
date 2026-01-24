/**
 * @file app_config.cpp
 * @brief Implementation of AppConfig
 * @author CaptureMoment Team
 * @date 2025
 */

#include "config/app_config.h"
#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::Config {

AppConfig& AppConfig::instance()
{
    // C++11 Magic Static: Thread-safe initialization.
    static AppConfig s_instance;
    return s_instance;
}

void AppConfig::setProcessingBackend(Common::MemoryType backend)
{
    if (m_processing_backend != backend) {
        m_processing_backend = backend;

        const auto backend_str = (backend == Common::MemoryType::CPU_RAM)
                                     ? "CPU_RAM"
                                     : "GPU_MEMORY";

        spdlog::info("AppConfig: Processing backend changed to {}.", backend_str);
    }
}

Common::MemoryType AppConfig::getProcessingBackend() const noexcept
{
    return m_processing_backend;
}

#ifdef ENABLE_TESTS
void AppConfig::reset()
{
    spdlog::debug("AppConfig: Resetting to defaults.");
    m_processing_backend = Common::MemoryType::CPU_RAM;
}
#endif

} // namespace CaptureMoment::Core::Config
