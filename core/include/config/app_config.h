/**
 * @file app_config.h
 * @brief Application-wide configuration singleton.
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "common/types/memory_type.h"

namespace CaptureMoment::Core{

namespace Config {

/**
 * @brief Singleton class for managing application-wide configuration settings.
 *
 * This class provides a global point of access to configuration values that are
 * determined at startup (e.g., by benchmarking or user preference) and remain
 * constant during the application's lifetime.
 *
 * @note While a Singleton is used here for convenience, in highly decoupled
 *       subsystems (e.g., `StateImageManager`), consider passing the configured
 *       values via Dependency Injection (Constructor) to improve testability.
 */

class AppConfig {
public:
    /**
     * @brief Gets the singleton instance of AppConfig.
     * Thread-safe (C++11 standard).
     */
    static AppConfig& instance();

    /**
     * @brief Sets the processing backend.
     * @param backend The chosen memory type (CPU_RAM or GPU_MEMORY).
     */
    void setProcessingBackend(Common::MemoryType backend);

    /**
     * @brief Gets the configured processing backend.
     * @return The MemoryType representing the chosen backend.
     */
    [[nodiscard]] Common::MemoryType getProcessingBackend() const noexcept;


    // ============================================================
    // Testing Utilities (Optional, but recommended for Seniors)
    // ============================================================
#ifdef ENABLE_TESTS // Defined via CMake when BUILD_TESTS is ON
    /**
     * @brief Resets the configuration to defaults.
     * Used exclusively in unit tests to ensure state isolation.
     * @warning Do not call this in production code.
     */
    void reset();
#endif

private:
    /**
     * @brief Private constructor to enforce singleton pattern.
     */
    AppConfig() = default;

    /**
     * @brief Deleted copy constructor to enforce singleton pattern.
     */
    AppConfig(const AppConfig&) = delete;

    /**
     * @brief Deleted assignment operator to enforce singleton pattern.
     */
    AppConfig& operator=(const AppConfig&) = delete;

    /**
     * @brief The selected hardware backend for image processing.
     */
    Common::MemoryType m_processing_backend{Common::MemoryType::CPU_RAM};
};

} // namespace Config

} // namespace CaptureMoment::Core

