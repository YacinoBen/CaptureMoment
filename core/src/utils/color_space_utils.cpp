/**
 * @file color_space_utils.cpp
 * @brief Implementation of color space analysis and conversion utilities.
 * @author CaptureMoment Team
 * @date 2026
 */

#include "utils/color_space_utils.h"

#include <OpenImageIO/imagebufalgo.h>
#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::Utils {

std::string getColorSpace(const OIIO::ImageBuf& buf) noexcept
{
    const auto cs { buf.spec().get_string_attribute("oiio:ColorSpace") };
    return cs.empty() ? std::string{} : std::string(cs);
}

bool isLinearToken(std::string_view colorspace) noexcept
{
    if (colorspace.empty()) {
        return false;
    }

    // All CIF linear tokens start with "lin_"
    // Examples: lin_rec709_scene, lin_ap1_scene, lin_ap0_scene, lin_srgb
    if (colorspace.starts_with("lin_")) {
        return true;
    }

    // OCIO role for scene-linear data
    if (colorspace.starts_with("scene_linear")) {
        return true;
    }

    // "Linear" is OIIO's shorthand for lin_rec709_scene
    if (colorspace == "Linear") {
        return true;
    }

    return false;
}

ColorSpaceInfo analyzeColorSpace(const OIIO::ImageSpec& spec) noexcept
{
    const auto cs { spec.get_string_attribute("oiio:ColorSpace") };
    
    if (cs.empty()) {
        return {false, ""};
    }

    return {isLinearToken(cs), std::string(cs)};
}

std::expected<void, ErrorHandling::CoreError>
transformToColorSpace(OIIO::ImageBuf& image, std::string_view target_cs)
{
    if (!image.initialized()) {
        spdlog::warn("[transformToColorSpace]: Buffer is not initialized");
        return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
    }

    const std::string source_cs { getColorSpace(image) };

    if (source_cs.empty()) {
        spdlog::warn("[transformToColorSpace]: No color space metadata found");
        return std::unexpected(ErrorHandling::CoreError::DecodingError);
    }

    if (!OIIO::ImageBufAlgo::colorconvert(image, image,
                                            source_cs,
                                            std::string(target_cs))) {
        spdlog::error("[transformToColorSpace]: Conversion from {} to {} failed: {}",
                      source_cs, target_cs, image.geterror());
        return std::unexpected(ErrorHandling::CoreError::DecodingError);
    }

    return {};
}

} // namespace CaptureMoment::Core::Utils
