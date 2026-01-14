/**
 * @file exiv2_provider.cpp
 * @brief Implementation of Exiv2Provider
 * @author CaptureMoment Team
 * @date 2025
 */


#include "serializer/provider/exiv2_provider.h"
#include "serializer/provider/exiv2_initializer.h"
#include <spdlog/spdlog.h>
#include <exiv2/exiv2.hpp>
#include <string>

namespace CaptureMoment::Core::Serializer {

std::string Exiv2Provider::readXmp(std::string_view file_path) const
{
    if (file_path.empty()) {
        spdlog::error("Exiv2Provider::readXmp: File path is empty.");
        return {};
    }

    spdlog::debug("Exiv2Provider::readXmp: Attempting to read XMP from file: {}", file_path);

    Exiv2Initializer::initialize();

    try {
        Exiv2::Image::UniquePtr image = Exiv2::ImageFactory::open(file_path.data());
        if (!image) {
            spdlog::warn("Exiv2Provider::readXmp: Failed to open image file with Exiv2: {}", file_path);
            return {};
        }

        image->readMetadata();

        Exiv2::XmpData& xmp_data = image->xmpData();

        if (xmp_data.empty()) {
            spdlog::debug("Exiv2Provider::readXmp: No XMP data found in file: {}", file_path);
            return {};
        }

        std::string xmp_packet;
        if (Exiv2::XmpParser::encode(xmp_packet, xmp_data) != 0) {
            spdlog::error("Exiv2Provider::readXmp: Failed to encode XMP data to packet for file: {}", file_path);
            return {};
        }

        spdlog::debug("Exiv2Provider::readXmp: Successfully read XMP packet (size {}) from file: {}", xmp_packet.size(), file_path);
        return xmp_packet;
    } catch (const Exiv2::Error& e) {
        spdlog::error("Exiv2Provider::readXmp: Exiv2 error reading XMP from file '{}': {}", file_path, e.what());
        return {};
    } catch (const std::exception& e) {
        spdlog::error("Exiv2Provider::readXmp: Standard exception reading XMP from file '{}': {}", file_path, e.what());
        return {};
    }
}

bool Exiv2Provider::writeXmp(std::string_view file_path, std::string_view xmp_data) const
{
    if (file_path.empty()) {
        spdlog::error("Exiv2Provider::writeXmp: File path is empty.");
        return false;
    }

    spdlog::debug("Exiv2Provider::writeXmp: Attempting to write XMP packet (size {}) to file: {}", xmp_data.size(), file_path);

    Exiv2Initializer::initialize();

    try {
        Exiv2::Image::UniquePtr image = Exiv2::ImageFactory::open(file_path.data());
        if (!image) {
            spdlog::error("Exiv2Provider::writeXmp: Failed to open image file with Exiv2 for writing: {}", file_path);
            return false;
        }

        image->readMetadata();

        Exiv2::XmpData xmp_data_container;
        if (!xmp_data.empty()) {
            if (Exiv2::XmpParser::decode(xmp_data_container, xmp_data.data()) != 0) {
                spdlog::error("Exiv2Provider::writeXmp: Failed to decode provided XMP packet for file: {}", file_path);
                return false;
            }
        }

        image->setXmpData(xmp_data_container);
        image->writeMetadata();

        spdlog::info("Exiv2Provider::writeXmp: Successfully wrote XMP packet to file: {}", file_path);
        return true;
    } catch (const Exiv2::Error& e) {
        spdlog::error("Exiv2Provider::writeXmp: Exiv2 error writing XMP to file '{}': {}", file_path, e.what());
        return false;
    } catch (const std::exception& e) {
        spdlog::error("Exiv2Provider::writeXmp: Standard exception writing XMP to file '{}': {}", file_path, e.what());
        return false;
    }
}

} // namespace CaptureMoment::Core::Serializer
