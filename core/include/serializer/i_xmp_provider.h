/**
 * @file i_xmp_provider.h
 * @brief Declaration of IXmpProvider interface
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <string>

namespace CaptureMoment::Core {

namespace Serializer {

/**
 * @brief Interface abstracting the service for reading/writing raw XMP packets to/from files.
 * This allows switching underlying XMP libraries (e.g., Exiv2, Adobe XMP Toolkit) without changing dependent code.
 */
class IXmpProvider {
public:
    virtual ~IXmpProvider() = default;

    /**
     * @brief Reads the raw XMP packet data from the specified file.
     * @param filePath The path to the file to read from.
     * @return The raw XMP packet string. An empty string if no XMP packet is found or an error occurs.
     */
    [[nodiscard]] virtual std::string readXmp(std::string_view filePath) const = 0;

    /**
     * @brief Writes the provided raw XMP packet data to the specified file.
     * The XMP packet should be a valid XMP string, typically starting with <?xpacket...?>.
     * @param filePath The path to the file to write to.
     * @param xmpData The raw XMP packet string to write.
     * @return true if the write operation was successful, false otherwise.
     */
    [[nodiscard]] virtual bool writeXmp(std::string_view filePath, std::string_view xmpData) const = 0;
};

} // namespace Serializer

} // namespace CaptureMoment::Core
