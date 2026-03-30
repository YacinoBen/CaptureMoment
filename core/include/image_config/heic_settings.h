/**
 * @file heic_settings.h
 * @brief Input configuration settings for HEIF/HEIC/AVIF image loading via OIIO/libheif.
 * @author CaptureMoment Team
 * @date 2026
 *
 * This file provides a configuration system for HEIF family formats
 * (HEIC based on HEVC/H.265, AVIF based on AV1) via OIIO/libheif.
 *
 * This struct is used exclusively by SourceManager for loading/reading.
 * Output settings (compression, quality, format choice) are handled
 * by the export module, where the user selects the target format.
 *
 * All parameters are runtime-configurable with sensible defaults.
 *
 * @note Currently OIIO's HEIF reader supports reading as RGB or RGBA, uint8 pixel values.
 *       HDR (more than 8 bits) support is planned for future libheif versions.
 *
 * @note libheif >= 1.16 required for "oiio:reorient" configuration option.
 * @note libheif >= 1.17 required for monochrome HEIC images.
 *
 * @see RawSettings For RAW-specific configuration.
 */

#pragma once

#include <string>

namespace CaptureMoment::Core::ImageConfig {

/**
 * @brief Namespace for HEIF/HEIC/AVIF-specific configuration settings.
 */
namespace Heic {

// ═══════════════════════════════════════════════════════════════════════════════
// ENUMERATIONS FOR HEIF SETTINGS
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * @enum AlphaMode
 * @brief Controls how unassociated alpha channels are handled during reading.
 *
 * Some HEIF files contain unassociated alpha, meaning the alpha channel
 * describes coverage but the color channels have not been pre-multiplied.
 * This setting determines whether to premultiply color channels by alpha
 * during loading, or leave them unassociated.
 *
 * @note The Core pipeline treats all channels identically (channel-agnostic).
 *       Premultiplied alpha is the recommended default because it keeps
 *       operations (brightness, contrast, etc.) consistent across all channels
 *       without requiring special alpha handling in Halide kernels.
 */
enum class AlphaMode : std::uint8_t {
    premultiply,       ///< Default OIIO behavior: premultiply color channels by alpha if unassociated.
    unassociated       ///< Leave alpha unassociated. Preserve original color values without modification.
};

// ═══════════════════════════════════════════════════════════════════════════════
// HEIF SETTINGS STRUCTURE (INPUT ONLY)
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * @struct HeicSettings
 * @brief Input configuration for HEIF/HEIC/AVIF image loading via OIIO/libheif.
 *
 * This struct encapsulates all HEIF **input** parameters used by SourceManager
 * when loading HEIF family files. Output settings (compression codec, quality,
 * format selection) are handled by the export module and are NOT part of this struct.
 *
 * ## Architecture Notes
 * - **Input only**: reorient, alpha_mode control how OIIO reads HEIF files.
 * - **Output excluded**: compression, quality, format choice belong to the export module.
 *   The user selects the output format at export time, not at load time.
 * - HEIF files from smartphones (iPhone, Android) are the most common use case.
 *
 * ## Default Configuration Rationale
 * - **reorient**: Enabled. HEIF files contain EXIF orientation metadata from smartphones.
 *                 Reorientation ensures correct display without manual rotation.
 * - **alpha_mode**: premultiply. Standard compositing convention, consistent with Core's
 *                    channel-agnostic F32 pipeline. Avoids special alpha handling in kernels.
 *
 * ## OIIO Limitations
 * - All pixel data is currently uint8. HDR support (10-bit, 12-bit) is planned.
 * - Multi-image files are supported for reading, not yet for writing.
 * - libheif >= 1.16 required for reorient option.
 * - libheif >= 1.17 required for monochrome HEIC images.
 *
 * @note Thread-safe for read access. Write operations should be synchronized externally.
 */
struct HeicSettings {

    // ═════════════════════════════════════════════════════════════════════════
    // INPUT SETTINGS (used by SourceManager)
    // ═════════════════════════════════════════════════════════════════════════

private:
    /**
     * @brief If nonzero, asks libheif to reorient images based on EXIF orientation.
     *
     * When enabled (default), libheif rotates/flips the image pixels to match
     * the orientation indicated by the camera, and sets the "Orientation"
     * metadata to 1 (meaning "no rotation needed"). The original orientation
     * is preserved in "oiio:OriginalOrientation" metadata.
     *
     * When disabled, the pixels are left in their stored orientation and
     * the "Orientation" metadata reflects the camera orientation.
     *
     * This is particularly important for HEIF files from smartphones, which
     * almost always store orientation metadata (portrait/landscape).
     *
     * @note Requires libheif >= 1.16.
     */
    bool m_reorient{true};

    /**
     * @brief Controls how unassociated alpha is handled during reading.
     *
     * HEIF files can contain unassociated alpha (alpha describes pixel coverage
     * but color channels are not pre-multiplied by alpha). The default OIIO
     * behavior is to premultiply color channels by alpha. Setting this to
     * unassociated preserves the original color values.
     *
     * For the Core's F32 processing pipeline, premultiplied alpha is the
     * recommended default because the pipeline is channel-agnostic:
     * all operations (brightness, contrast, shadows, etc.) treat all 4 channels
     * identically. Premultiplied alpha ensures that operations on RGB channels
     * remain consistent with the alpha channel without requiring special handling.
     *
     * @note Setting this to unassociated is only safe if the pipeline explicitly
     *       excludes the alpha channel from processing operations.
     */
    AlphaMode m_alpha_mode{AlphaMode::premultiply};

public:
    /**
     * @brief Checks if automatic reorientation is enabled.
     * @return true if libheif will reorient images based on EXIF orientation.
     */
    [[nodiscard]] constexpr bool get_reorient() const noexcept { return m_reorient; }

    /**
     * @brief Enables or disables automatic reorientation.
     * @param reorient true to reorient based on EXIF orientation (recommended).
     * @note Requires libheif >= 1.16.
     */
    constexpr void set_reorient(bool reorient) noexcept { m_reorient = reorient; }

    /**
     * @brief Gets the reorient value as integer (for OIIO attribute).
     * @return 1 if reorient enabled, 0 if disabled.
     */
    [[nodiscard]] constexpr int get_reorient_value() const noexcept {
        return m_reorient ? 1 : 0;
    }

    /**
     * @brief Gets the alpha handling mode.
     * @return The current alpha mode.
     */
    [[nodiscard]] constexpr AlphaMode get_alpha_mode() const noexcept { return m_alpha_mode; }

    /**
     * @brief Sets the alpha handling mode.
     * @param mode The alpha mode to use.
     */
    constexpr void set_alpha_mode(AlphaMode mode) noexcept { m_alpha_mode = mode; }

    /**
     * @brief Gets the unassociated alpha value as integer (for OIIO attribute).
     * @return 1 if unassociated alpha should be preserved, 0 for premultiply (default).
     */
    [[nodiscard]] constexpr int get_unassociated_alpha_value() const noexcept {
        return (m_alpha_mode == AlphaMode::unassociated) ? 1 : 0;
    }

    // ═════════════════════════════════════════════════════════════════════════
    // PRESET FACTORY METHODS
    // ═════════════════════════════════════════════════════════════════════════

    /**
     * @brief Creates a HeicSettings configured for maximum processing speed.
     * @return HeicSettings with speed-optimized configuration.
     *
     * ## Configuration Rationale
     * | Parameter     | Value           | Reason                                  |
     * |---------------|-----------------|-----------------------------------------|
     * | reorient      | false           | Skip pixel reorientation overhead       |
     * | alpha_mode    | premultiply     | Standard convention, no extra work      |
     *
     * @note Use for quick preview loading or when speed is critical.
     * @warning Image may appear rotated incorrectly if reorient is disabled.
     */
    [[nodiscard]] static constexpr HeicSettings fast_settings() noexcept {
        HeicSettings settings;
        settings.m_reorient = false;
        settings.m_alpha_mode = AlphaMode::premultiply;
        return settings;
    }

    /**
     * @brief Creates a HeicSettings with default values.
     * @return HeicSettings with recommended default configuration.
     *
     * ## Configuration Rationale
     * | Parameter     | Value           | Reason                                  |
     * |---------------|-----------------|-----------------------------------------|
     * | reorient      | true            | Correct orientation from EXIF metadata  |
     * | alpha_mode    | premultiply     | Consistent with channel-agnostic pipeline|
     *
     * @note This is the recommended configuration for all standard use cases.
     */
    [[nodiscard]] static constexpr HeicSettings default_settings() noexcept {
        return HeicSettings{};
    }

    // ═════════════════════════════════════════════════════════════════════════
    // UTILITY METHODS
    // ═════════════════════════════════════════════════════════════════════════

    /**
     * @brief Resets all settings to their default values.
     */
    void reset_to_defaults() noexcept {
        *this = HeicSettings{};
    }
};

} // namespace Heic

} // namespace CaptureMoment::Core::ImageConfig
