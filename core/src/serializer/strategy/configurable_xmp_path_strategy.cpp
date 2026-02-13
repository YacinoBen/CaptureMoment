/**
 * @file configurable_xmp_path_strategy.cpp
 * @brief Implementation of ConfigurableXmpPathStrategy
 * @author CaptureMoment Team
 * @date 2025
 */

#include "serializer/strategy/configurable_xmp_path_strategy.h"
#include <spdlog/spdlog.h>
#include <exiv2/exiv2.hpp>
#include <filesystem>

namespace CaptureMoment::Core::Serializer {

ConfigurableXmpPathStrategy::ConfigurableXmpPathStrategy(const std::string& base_xmp_dir, std::unique_ptr<IXmpProvider> xmp_provider)
    : m_base_xmp_dir(base_xmp_dir)
    , m_xmp_provider(std::move(xmp_provider))
{
    if (!m_xmp_provider) {
        spdlog::error("ConfigurableXmpPathStrategy: Constructor received a null IXmpProvider.");
        throw std::invalid_argument("ConfigurableXmpPathStrategy: IXmpProvider cannot be null.");
    }

    if (!std::filesystem::exists(m_base_xmp_dir)) {
        try {
            std::filesystem::create_directories(m_base_xmp_dir);
            spdlog::info("ConfigurableXmpPathStrategy: Created configured base XMP directory: {}", m_base_xmp_dir);
        } catch (const std::exception& e) {
            spdlog::error("ConfigurableXmpPathStrategy: Failed to create configured base XMP directory '{}': {}", m_base_xmp_dir, e.what());
        }
    }
    spdlog::debug("ConfigurableXmpPathStrategy: Initialized with base XMP directory: {} and IXmpProvider.", m_base_xmp_dir);
}

std::string ConfigurableXmpPathStrategy::getXmpPathForImage(std::string_view source_image_path) const
{
    if (source_image_path.empty()) {
        spdlog::error("ConfigurableXmpPathStrategy::getXmpPathForImage: Source image path is empty.");
        return {};
    }

    std::filesystem::path image_path_obj(source_image_path);

    // Calculate relative path
    // NOTE: Assumes 'source_image_path' is absolute.
    std::filesystem::path relative_path = image_path_obj.lexically_relative(image_path_obj.root_path());

    // Construct target path in configured directory, mirroring structure
    std::filesystem::path xmp_final_path = m_base_xmp_dir / relative_path;
    xmp_final_path += ".xmp";

    spdlog::debug("ConfigurableXmpPathStrategy::getXmpPathForImage: Mapped '{}' to XMP path: '{}'", source_image_path, xmp_final_path.string());
    return xmp_final_path.string();
}

std::string ConfigurableXmpPathStrategy::getImagePathFromXmp(std::string_view xmp_path) const
{
    if (xmp_path.empty()) {
        spdlog::error("ConfigurableXmpPathStrategy::getImagePathFromXmp: XMP path is empty.");
        return {};
    }

    spdlog::debug("ConfigurableXmpPathStrategy::getImagePathFromXmp: Attempting to read original image path from XMP: {}", xmp_path);

    // Read the XMP packet to find the stored path
    std::string xmp_packet = m_xmp_provider->readXmp(xmp_path);
    if (xmp_packet.empty()) {
        spdlog::warn("ConfigurableXmpPathStrategy::getImagePathFromXmp: IXmpProvider returned an empty XMP packet for file: {}", xmp_path);
        return {};
    }

    Exiv2::XmpData xmp_data;
    try {
        if (Exiv2::XmpParser::decode(xmp_data, xmp_packet.c_str()) != 0) {
            spdlog::error("ConfigurableXmpPathStrategy::getImagePathFromXmp: Failed to decode XMP packet from file: {}", xmp_path);
            return {};
        }

        // Extract the stored source path
        std::string source_image_path_from_xmp = xmp_data["Xmp.cm.sourceImagePath"].toString();

        if (source_image_path_from_xmp.empty()) {
            spdlog::warn("ConfigurableXmpPathStrategy::getImagePathFromXmp: XMP packet in file '{}' does not contain the 'Xmp.cm.sourceImagePath' key.", xmp_path);
        } else {
            spdlog::debug("ConfigurableXmpPathStrategy::getImagePathFromXmp: Found source image path in XMP: '{}'", source_image_path_from_xmp);
        }

        return source_image_path_from_xmp;

    } catch (const Exiv2::Error& e) {
        spdlog::error("ConfigurableXmpPathStrategy::getImagePathFromXmp: Exiv2 error during parsing XMP file '{}': {}", xmp_path, e.what());
        return {};
    } catch (const std::exception& e) {
        spdlog::error("ConfigurableXmpPathStrategy::getImagePathFromXmp: General error during parsing XMP file '{}': {}", xmp_path, e.what());
        return {};
    }
}

} // namespace CaptureMoment::Core::Serializer
