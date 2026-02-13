/**
 * @file sidecar_xmp_path_strategy.cpp
 * @brief Implementation of SidecarXmpPathStrategy
 * @details Logic for handling sidecar XMP files (stored next to the image).
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
        return {};
    }

    std::filesystem::path image_path_obj(source_image_path);

    // Append ".xmp" to the existing filename in the same directory
    std::filesystem::path xmp_path_obj = image_path_obj;
    xmp_path_obj += ".xmp";

    spdlog::debug("SidecarXmpPathStrategy::getXmpPathForImage: Mapped '{}' to XMP path: '{}'", source_image_path, xmp_path_obj.string());
    return xmp_path_obj.string();
}

std::string SidecarXmpPathStrategy::getImagePathFromXmp(std::string_view xmp_path) const
{
    if (xmp_path.empty()) {
        spdlog::error("SidecarXmpPathStrategy::getImagePathFromXmp: XMP path is empty.");
        return {};
    }

    std::filesystem::path xmp_path_obj(xmp_path);
    std::string xmp_ext = xmp_path_obj.extension().string();

    // Check if the file ends with ".xmp"
    if (xmp_ext.size() >= 4 && xmp_ext.substr(xmp_ext.size() - 4) == ".xmp")
    {
        // Remove the last 4 characters (.xmp) to reconstruct the original filename
        std::string original_ext = xmp_ext.substr(0, xmp_ext.size() - 4);
        std::string original_stem = xmp_path_obj.stem().string(); // Stem includes filename without extension
        std::string reconstructed_name = original_stem + original_ext;

        std::filesystem::path original_path = xmp_path_obj.parent_path() / reconstructed_name;
        spdlog::debug("SidecarXmpPathStrategy::getImagePathFromXmp: Reconstructed image path '{}' from XMP path: '{}'", original_path.string(), xmp_path);
        return original_path.string();
    } else {
        spdlog::warn("SidecarXmpPathStrategy::getImagePathFromXmp: XMP path does not end with '.xmp': '{}'", xmp_path);
        return {};
    }
}

} // namespace CaptureMoment::Core::Serializer
