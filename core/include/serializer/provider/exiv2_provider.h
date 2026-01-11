/**
 * @file exiv2_provider.h
 * @brief Declaration of Exiv2Provider class
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include "serializer/provider/i_xmp_provider.h"
#include <string_view>
namespace CaptureMoment::Core {

namespace Serializer {

/**
 * @brief Concrete implementation of IXmpProvider using the Exiv2 library.
 * This class handles the low-level interaction with Exiv2 to read and write XMP packets.
 */
class Exiv2Provider final : public IXmpProvider {
public:
    /**
     * @brief Constructs an Exiv2Provider.
     * Note: Exiv2 library itself must be initialized separately using Exiv2Initializer.
     */
    Exiv2Provider() = default;
    ~Exiv2Provider() override = default;

    // Delete copy/move to ensure consistent state management if needed later
    // (Currently not strictly necessary if Exiv2 itself is globally stateless for basic ops)
    Exiv2Provider(const Exiv2Provider&) = delete;
    Exiv2Provider& operator=(const Exiv2Provider&) = delete;
    Exiv2Provider(Exiv2Provider&&) = delete;
    Exiv2Provider& operator=(Exiv2Provider&&) = delete;

    /**
     * @brief Reads the raw XMP packet data from the specified file using Exiv2.
     * @param filePath The path to the file to read from.
     * @return The raw XMP packet string. An empty string if no XMP packet is found or an error occurs.
     */
    [[nodiscard]] std::string readXmp(std::string_view file_path) const override;

    /**
     * @brief Writes the provided raw XMP packet data to the specified file using Exiv2.
     * The XMP packet should be a valid XMP string, typically starting with <?xpacket...?>.
     * @param filePath The path to the file to write to.
     * @param xmpData The raw XMP packet string to write.
     * @return true if the write operation was successful, false otherwise.
     */
    [[nodiscard]] bool writeXmp(std::string_view file_path, std::string_view xmp_data) const override;
};

} // namespace Serializer

} // namespace CaptureMoment::Core
