/**
 * @file viewport_config.h
 * @brief Configuration constants and helper functions for viewport management.
 * @author CaptureMoment Team
 * @date 2026
 * 
 * This file provides screen-based limits and quality settings for optimal
 * display across all screen types. The configuration is based on physical
 * screen characteristics, not device categories.
 */

#pragma once

#include <QSize>
#include <QtGlobal>
#include <algorithm>

namespace CaptureMoment::UI {
namespace Display {

/**
 * @struct DisplayConfig
 * @brief Compile-time configuration constants for display management.
 * 
 * All values are evaluated at compile-time using consteval.
 * This ensures zero runtime overhead and compile-time verification.
 */
struct DisplayConfig {
    // =========================================================================
    // Size Limits
    // =========================================================================

    /**
     * @brief Minimum display dimension to prevent too small textures.
     * @return Minimum dimension in pixels (256).
     */
    [[nodiscard]] consteval static int minDisplayDimension() noexcept {
        return 256;
    }

    /**
     * @brief Plafond (ceiling) for small physical screens.
     * 
     * Screens with physical size < 2000px use this plafond.
     * Value: 2K (2048px) - suitable for most laptops, tablets, phones.
     * 
     * @return Small screen plafond in pixels (2048).
     */
    [[nodiscard]] consteval static int plafondSmallScreen() noexcept {
        return 2048;
    }

    /**
     * @brief Plafond (ceiling) for large physical screens.
     * 
     * Screens with physical size >= 2000px use this plafond.
     * Value: 3K (3072px) - suitable for 4K monitors, 5K iMac, etc.
     * 
     * @return Large screen plafond in pixels (3072).
     */
    [[nodiscard]] consteval static int plafondLargeScreen() noexcept {
        return 3072;
    }

    // =========================================================================
    // Quality Settings
    // =========================================================================

    /**
     * @brief Default quality margin for zoom headroom.
     * 
     * Multiplier applied to viewport size to allow zooming before
     * pixelation becomes visible. A value of 1.25 means 25% zoom headroom.
     * 
     * @return Default quality margin (1.25).
     */
    [[nodiscard]] consteval static float defaultQualityMargin() noexcept {
        return 1.25f;
    }

    /**
     * @brief Minimum quality margin.
     * 
     * No headroom, texture exactly matches viewport physical size.
     * 
     * @return Minimum quality margin (1.0).
     */
    [[nodiscard]] consteval static float minQualityMargin() noexcept {
        return 1.0f;
    }

    /**
     * @brief Maximum quality margin.
     * 
     * 100% headroom, allows 2× zoom before pixelation.
     * 
     * @return Maximum quality margin (2.0).
     */
    [[nodiscard]] consteval static float maxQualityMargin() noexcept {
        return 2.0f;
    }

    // =========================================================================
    // Screen Classification Thresholds
    // =========================================================================

    /**
     * @brief Threshold for large screen classification.
     * 
     * Screens with width or height >= this value are considered "large"
     * and use plafondLargeScreen().
     * 
     * @return Large screen threshold in pixels (2000).
     */
    [[nodiscard]] consteval static int largeScreenThreshold() noexcept {
        return 2000;
    }

    /**
     * @brief Minimum DPR for HiDPI classification.
     * 
     * Screens with DPR >= this value are considered HiDPI (Retina, etc.).
     * 
     * @return HiDPI DPR threshold (2.0).
     */
    [[nodiscard]] consteval static float hidpiDprThreshold() noexcept {
        return 2.0f;
    }
};

// =============================================================================
// Helper Functions
// =============================================================================

/**
 * @brief Calculates texture memory usage in megabytes.
 * 
 * @param size The texture size in pixels.
 * @param bytes_per_pixel Bytes per pixel (default 4 for RGBA8).
 * @return Memory usage in megabytes (MB).
 */
inline size_t calculateTextureMemoryMB(const QSize& size, int bytes_per_pixel = 4) noexcept
{
    if (!size.isValid()) {
        return 0;
    }
    return static_cast<size_t>(size.width()) * 
           static_cast<size_t>(size.height()) * 
           static_cast<size_t>(bytes_per_pixel) / 
           static_cast<size_t>(1024 * 1024);
}

/**
 * @brief Determines the plafond based on physical screen size.
 * 
 * @param screen_physical_width Physical screen width in pixels.
 * @param screen_physical_height Physical screen height in pixels.
 * @return The appropriate plafond (plafondSmallScreen or plafondLargeScreen).
 */
inline int determinePlafond(int screen_physical_width, int screen_physical_height) noexcept
{
    const int max_dimension = std::max(screen_physical_width, screen_physical_height);
    
    if (max_dimension >= DisplayConfig::largeScreenThreshold()) {
        return DisplayConfig::plafondLargeScreen();
    }
    return DisplayConfig::plafondSmallScreen();
}

} // namespace Display
} // namespace CaptureMoment::UI
