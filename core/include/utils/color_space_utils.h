/**
 * @file color_space_utils.h
 * @brief Utilities for analyzing and converting image color spaces.
 * @author CaptureMoment Team
 * @date 2026
 *
 * Provides functions to:
 * - Detect if an image's pixel data is linear via OIIO metadata.
 * - Convert an image to a target color space via OIIO colorconvert.
 *
 * OIIO automatically detects and stores the color space in the
 * "oiio:ColorSpace" ImageSpec attribute when reading a file.
 * These utilities read that metadata — no manual file inspection needed.
 */

#pragma once

#include <OpenImageIO/imagebuf.h>
#include "common/error_handling/core_error.h"

#include <expected>
#include <string>
#include <utility>

namespace CaptureMoment::Core::Utils {

/**
 * @brief Result of a color space analysis.
 *
 * First element: `true` if the image data uses a linear transfer function.
 * Second element: the detected color space name (CIF token).
 *               Empty if OIIO could not determine it.
 */
using ColorSpaceInfo = std::pair<bool, std::string>;

/**
 * @brief Checks whether an OIIO-detected color space token represents linear data.
 *
 * Matches against known CIF (Color Interop Forum) linear tokens:
 * - "lin_rec709_scene" (synonyms: "lin_rec709", "lin_srgb", "Linear")
 * - "lin_ap1_scene"     (synonym: "ACEScg")
 * - "lin_ap0_scene"
 * - "scene_linear"      (OCIO role)
 *
 * @param colorspace The color space name from OIIO metadata.
 * @return true if the token represents a linear transfer function.
 */
[[nodiscard]] bool isLinearToken(std::string_view colorspace) noexcept;

/**
 * @brief Analyzes whether an image's pixel data is stored in a linear color space.
 *
 * Reads the "oiio:ColorSpace" attribute that OIIO sets automatically
 * during file reading. Does not read the file — uses OIIO's metadata only.
 *
 * @param spec The OIIO ImageSpec to inspect.
 * @return ColorSpaceInfo with linear status and detected color space name.
 *
 * @note If OIIO did not set "oiio:ColorSpace" (e.g., BMP, ICO without metadata),
 *       returns {false, ""}. The caller should apply a fallback strategy.
 */
[[nodiscard]] ColorSpaceInfo analyzeColorSpace(const OIIO::ImageSpec& spec) noexcept;

/**
 * @brief Converts an image buffer to a target color space (in-place).
 *
 * Reads the source color space from the buffer's OIIO metadata
 * ("oiio:ColorSpace") and converts to the specified target.
 *
 * @param image The image buffer to convert (modified in-place).
 * @param target_cs The target color space CIF token (e.g., "lin_rec709_scene").
 * @return Success or CoreError on failure.
 */
[[nodiscard]] std::expected<void, ErrorHandling::CoreError>
transformToColorSpace(OIIO::ImageBuf& image, std::string_view target_cs);

/**
 * @brief Extracts the color space name from an OIIO image buffer metadata.
 * @return The detected color space name, or empty string if not found.
 */
[[nodiscard]] std::string getColorSpace(const OIIO::ImageBuf& buf) noexcept;

} // namespace CaptureMoment::Core::Utils
