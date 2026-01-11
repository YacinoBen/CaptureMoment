/**
 * @file sidecar_xmp_path_strategy.cpp
 * @brief Implementation of SidecarDataXmpPathStrategy
 * @author CaptureMoment Team
 * @date 2025
 */

#include "serializer/strategy/sidecar_xmp_path_strategy.h"
#include <spdlog/spdlog.h>
#include <filesystem>

namespace CaptureMoment::Core::Serializer {

std::string SidecarXmpPathStrategy::getXmpPathForImage(std::string_view source_image_path) const
{
    if (source_image_path.empty()) {
        spdlog::error("SidecarXmpPathStrategy::getXmpPathForImage: Source image path is empty.");
        return {}; // Return an empty string
    }

    std::filesystem::path image_path_obj {source_image_path};
    // Add ".xmp" to the existing extension
    std::string xmp_path_str = image_path_obj.parent_path().string() + "/" + image_path_obj.stem().string() + image_path_obj.extension().string() + ".xmp";

    spdlog::debug("SidecarXmpPathStrategy::getXmpPathForImage: Mapped '{}' to XMP path: '{}'", source_image_path, xmp_path_str);
    return xmp_path_str;
}

std::string SidecarXmpPathStrategy::getImagePathFromXmp(std::string_view xmp_path) const
{
    if (xmp_path.empty()) {
        spdlog::error("SidecarXmpPathStrategy::getImagePathFromXmp: XMP path is empty.");
        return {}; // Return an empty string
    }

    std::filesystem::path xmp_path_obj {xmp_path};
    std::string xmp_ext = xmp_path_obj.extension().string();

    // Check if the file ends with ".xmp"
    if (xmp_ext.size() >= 4 && xmp_ext.substr(xmp_ext.size() - 4) == ".xmp")
    {
        // Remove the last 4 characters (.xmp)
        std::string original_ext = xmp_ext.substr(0, xmp_ext.size() - 4);
        std::string original_stem = xmp_path_obj.stem().string(); // The stem might contain the original extension if it was .jpg.xmp
        std::string reconstructed_name = original_stem + original_ext; // Reconstruct the original name
        std::filesystem::path original_path = xmp_path_obj.parent_path() / reconstructed_name;
        spdlog::debug("SidecarXmpPathStrategy::getImagePathFromXmp: Reconstructed image path '{}' from XMP path: '{}'", original_path.string(), xmp_path);
        return original_path.string();
    } else {
        spdlog::warn("SidecarXmpPathStrategy::getImagePathFromXmp: XMP path does not end with '.xmp': '{}'", xmp_path);
        return {};
    }
}

} // namespace CaptureMoment::Core::Serializer
