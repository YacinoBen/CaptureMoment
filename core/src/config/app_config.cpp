/**
 * @file app_config.cpp
 * @brief Implementation of AppConfig
 * @author CaptureMoment Team
 * @date 2026
 */

#include "config/app_config.h"
#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::Config {

AppConfig& AppConfig::instance()
{
    static AppConfig instance;
    return instance;
}

void AppConfig::setProcessingBackend(Common::MemoryType backend)
{
    m_processing_backend = backend;
    spdlog::info("AppConfig: Processing backend set to {}.",
                 (backend == Common::MemoryType::CPU_RAM) ? "CPU" : "GPU");
}

Common::MemoryType AppConfig::getProcessingBackend() const
{
    return m_processing_backend;
}

} // namespace CaptureMoment::Core::Config
