/**
 * @file i_xmp_provider.h
 * @brief Declaration of IXmpProvider interface
 * @details Abstracts the raw XMP packet reading/writing logic to allow swapping the underlying library.
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <string>

namespace CaptureMoment::Core {

namespace Serializer {

/**
 * @brief Interface abstracting the service for reading/writing raw XMP packets to/from files.
 *
 * This interface allows switching underlying XMP libraries (e.g., Exiv2, Adobe XMP Toolkit)
 * without changing dependent code. It deals with the raw XMP string representation,
 * typically the XML-like packet starting with <?xpacket...?>.
 */
class IXmpProvider {
public:
    /**
     * @brief Virtual destructor for safe inheritance.
     */
    virtual ~IXmpProvider() = default;

    /**
     * @brief Reads the raw XMP packet data from the specified file.
     *
     * @param filePath The path to the file to read from.
     * @return The raw XMP packet string. Returns an empty string if no XMP packet is found or an error occurs.
     */
    [[nodiscard]] virtual std::string readXmp(std::string_view file_path) const = 0;

    /**
     * @brief Writes the provided raw XMP packet data to the specified file.
     *
     * The XMP packet should be a valid XMP string. The implementation typically handles
     * opening the file, parsing existing metadata, replacing the XMP section, and writing back to disk.
     *
     * @param filePath The path to the file to write to.
     * @param xmpData The raw XMP packet string to write.
     * @return true if the write operation was successful, false otherwise.
     */
    [[nodiscard]] virtual bool writeXmp(std::string_view filePath, std::string_view xmpData) const = 0;
};

} // namespace Serializer

} // namespace CaptureMoment::Core
