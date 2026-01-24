/**
 * @file app_config.h
 * @brief Application-wide configuration singleton.
 *
 * This header defines the @ref CaptureMoment::Core::Config::AppConfig class,
 * which serves as the central point of configuration for the application.
 * It manages global settings such as the active processing backend (CPU/GPU)
 * and the concrete Halide target configuration (CUDA, Metal, etc.).
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "common/types/memory_type.h"
#include <Halide.h>

namespace CaptureMoment::Core {

namespace Config {

/**
 * @class AppConfig
 * @brief Singleton class for managing application-wide configuration settings.
 *
 * @details
 * This class provides a global point of access to configuration values that are
 * determined at startup (e.g., by benchmarking or user preference) and remain
 * constant during the application's lifetime.
 *
 * It follows the Meyer's Singleton pattern for thread-safe initialization.
 *
 * **Managed Settings:**
 * 1. **Memory Backend**: Whether the processing pipeline uses CPU RAM or GPU Memory
 *    (Common::MemoryType).
 * 2. **Halide Target**: The specific Halide target object (e.g., configured for CUDA,
 *    Vulkan, or Metal) to be used by all Halide operations. This ensures that
 *    once a backend is chosen by IBackendDecider, all image processing components
 *    use the same optimized target.
 *
 * @note While a Singleton is used here for convenience, in highly decoupled
 *       subsystems (e.g., StateImageManager), consider passing the configured
 *       values via Dependency Injection (Constructor) to improve testability.
 */
class AppConfig {
public:
    /**
     * @brief Gets the singleton instance of AppConfig.
     *
     * @details
     * This method is thread-safe for initialization (C++11 Magic Static).
     *
     * @return A reference to the single AppConfig instance.
     */
    static AppConfig& instance();

    /**
     * @brief Sets the processing backend (CPU or GPU) for image operations.
     *
     * @details
     * This method updates the global configuration preference.
     * Typically called during the startup phase after `IBackendDecider` has
     * evaluated the best available backend.
     *
     * @param backend The chosen memory type (CPU_RAM or GPU_MEMORY).
     */
    void setProcessingBackend(Common::MemoryType backend);

    /**
     * @brief Gets the configured processing backend.
     *
     * @details
     * This method can be called from anywhere in the codebase to determine
     * where data should be stored or processed.
     *
     * @return The MemoryType representing the chosen backend (CPU_RAM or GPU_MEMORY).
     */
    [[nodiscard]] Common::MemoryType getProcessingBackend() const noexcept;

    /**
     * @brief Sets the configured Halide Target object.
     *
     * @details
     * This method stores the concrete `Halide::Target` object that should be used
     * by all Halide operations. The target contains information about the OS,
     * architecture, and specific GPU features (e.g., CUDA, Vulkan) that have
     * been enabled.
     *
     * This is typically called once by the `IBackendDecider` after it has
     * successfully initialized a GPU device or selected the default Host target.
     *
     * @param target The Halide::Target object to use for all compilations.
     */
    static void setHalideTarget(const Halide::Target& target);

    /**
     * @brief Gets the configured Halide Target object.
     *
     * @details
     * Retrieves the target set by `setHalideTarget`.
     * This target should be passed to Halide pipelines to ensure they compile
     * for the correct hardware (e.g., generating CUDA kernels instead of x86 code).
     *
     * @return A const reference to the configured Halide::Target object.
     */
    [[nodiscard]] static const Halide::Target& getHalideTarget();

    // ============================================================
    // Testing Utilities
    // ============================================================
#ifdef ENABLE_TESTS // Defined via CMake when BUILD_TESTS is ON
    /**
     * @brief Resets the configuration to defaults.
     *
     * @details
     * Used exclusively in unit tests to ensure state isolation.
     * Resets the backend to CPU_RAM and the Halide target to default host.
     *
     * @warning Do not call this method in production code.
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
     *
     * @details
     * Enum value indicating whether image data should reside in main
     * CPU RAM or in GPU Memory. Influences how `WorkingImageFactory`
     * creates working image instances.
     */
    Common::MemoryType m_processing_backend{Common::MemoryType::CPU_RAM};

    /**
     * @brief The active Halide Target object for the application.
     *
     * @details
     * This static member stores the concrete Halide configuration.
     * It is initialized to the default host target and then potentially updated
     * by `BenchmarkingBackendDecider` to include GPU features.
     */
    static Halide::Target s_halide_target;
};

} // namespace Config

} // namespace CaptureMoment::Core
