/**
 * @file exiv2_provider.h
 * @brief Declaration of Exiv2Provider class
 * @details Concrete implementation of IXmpProvider using the Exiv2 library.
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include "i_xmp_provider.h"

namespace CaptureMoment::Core {

namespace Serializer {

/**
 * @brief Concrete implementation of IXmpProvider using the Exiv2 library.
 *
 * This class handles the low-level file operations required to read and write XMP packets
 * embedded in or associated with image files (JPEG, TIFF, PNG, etc.).
 *
 * @note Marked as `final` to prevent inheritance and allow for potential compiler optimizations.
 */
class Exiv2Provider final : public IXmpProvider {
public:
    /**
     * @brief Default constructor.
     */
    Exiv2Provider() = default;

    /**
     * @brief Reads the raw XMP packet data from the specified file.
     *
     * @param filePath The path to the file to read from.
     * @return The raw XMP packet string. Returns an empty string if no XMP packet is found or an error occurs.
     */
    [[nodiscard]] std::string readXmp(std::string_view file_path) const override;

    /**
     * @brief Writes the provided raw XMP packet data to the specified file.
     *
     * This method reads the existing metadata to ensure non-XMP data (EXIF, IPTC) is preserved,
     * replaces the XMP section with the provided packet, and writes the metadata back to the file.
     *
     * @param filePath The path to the file to write to.
     * @param xmpData The raw XMP packet string to write.
     * @return true if the write operation was successful, false otherwise.
     */
    [[nodiscard]] bool writeXmp(std::string_view filePath, std::string_view xmpData) const override;
};

} // namespace Serializer

} // namespace CaptureMoment::Core
