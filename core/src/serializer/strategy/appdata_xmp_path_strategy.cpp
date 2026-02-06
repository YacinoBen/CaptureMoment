/**
 * @file appdata_xmp_path_strategy.cpp
 * @brief Implementation of AppDataXmpPathStrategy
 * @author CaptureMoment Team
 * @date 2025
 */

#include "serializer/strategy/appdata_xmp_path_strategy.h"
#include <spdlog/spdlog.h>
#include <exiv2/exiv2.hpp>
#include <filesystem>

namespace CaptureMoment::Core::Serializer {

AppDataXmpPathStrategy::AppDataXmpPathStrategy(const std::string& app_data_dir)
    : m_app_data_dir(app_data_dir)
{
    // Ensure the base directory exists
    if (!std::filesystem::exists(m_app_data_dir)) {
        try {
            std::filesystem::create_directories(m_app_data_dir);
            spdlog::info("AppDataXmpPathStrategy: Created AppData base directory: {}", m_app_data_dir);
        } catch (const std::exception& e) {
            spdlog::error("AppDataXmpPathStrategy: Failed to create AppData base directory '{}': {}", m_app_data_dir, e.what());
        }
    }
    spdlog::debug("AppDataXmpPathStrategy: Initialized with AppData directory: {}", m_app_data_dir);
}

std::string AppDataXmpPathStrategy::getXmpPathForImage(std::string_view source_image_path) const
{
    if (source_image_path.empty()) {
        spdlog::error("AppDataXmpPathStrategy::getXmpPathForImage: Source image path is empty.");
        return {};
    }

    std::filesystem::path image_path_obj(source_image_path);

    // Calculate the relative path from the root of the filesystem
    // NOTE: This logic assumes 'source_image_path' is an absolute path.
    // If it's relative, root_path() might be empty or misleading.
    std::filesystem::path relative_path = image_path_obj.lexically_relative(image_path_obj.root_path());

    // Construct the target path in AppData
    // Preserves folder structure: /home/user/pics/img.jpg -> <appdata>/home/user/pics/img.jpg.xmp
    std::filesystem::path xmp_final_path = m_app_data_dir / relative_path;
    xmp_final_path += ".xmp";

    spdlog::debug("AppDataXmpPathStrategy::getXmpPathForImage: Mapped '{}' to XMP path: '{}'", source_image_path, xmp_final_path.string());
    return xmp_final_path.string();
}

std::string AppDataXmpPathStrategy::getImagePathFromXmp(std::string_view xmp_path) const
{
    if (xmp_path.empty()) {
        spdlog::error("AppDataXmpPathStrategy::getImagePathFromXmp: XMP path is empty.");
        return {};
    }

    spdlog::debug("AppDataXmpPathStrategy::getImagePathFromXmp: Attempting to read original image path from XMP: {}", xmp_path);

    // Step 1: Read the XMP packet from the file using the injected provider
    std::string xmp_packet = m_xmp_provider->readXmp(xmp_path);
    if (xmp_packet.empty()) {
        spdlog::warn("AppDataXmpPathStrategy::getImagePathFromXmp: IXmpProvider returned an empty XMP packet for file: {}", xmp_path);
        return {};
    }

    // Step 2: Parse the XMP packet to find the source image path
    Exiv2::XmpData xmp_data;
    try {
        // Decode the XMP packet string into the XmpData container
        if (Exiv2::XmpParser::decode(xmp_data, xmp_packet.c_str()) != 0) {
            spdlog::error("AppDataXmpPathStrategy::getImagePathFromXmp: Failed to decode XMP packet from file: {}", xmp_path);
            return {}; // Return an empty string in case of decoding failure
        }

        // Read the source image path stored in the XMP
        std::string source_image_path_from_xmp = xmp_data["Xmp.cm.sourceImagePath"].toString(); // Returns an empty string if the key does not exist

        if (source_image_path_from_xmp.empty()) {
            spdlog::warn("AppDataXmpPathStrategy::getImagePathFromXmp: XMP packet in file '{}' does not contain the 'Xmp.cm.sourceImagePath' key.", xmp_path);
        } else {
            spdlog::debug("AppDataXmpPathStrategy::getImagePathFromXmp: Found source image path in XMP: '{}'", source_image_path_from_xmp);
        }

        return source_image_path_from_xmp;

    } catch (const Exiv2::Error& e) {
        spdlog::error("Context::AppDataXmpPathStrategy::getImagePathFromXmp: Exiv2 error during parsing XMP file '{}': {}", xmp_path, e.what());
        return {};
    } catch (const std::exception& e) {
        spdlog::error("Context::AppDataXmpPathStrategy::getImagePathFromXmp: General error during parsing XMP file '{}': {}", xmp_path, e.what());
        return {};
    }
}

} // namespace CaptureMoment::Core::Serializer
