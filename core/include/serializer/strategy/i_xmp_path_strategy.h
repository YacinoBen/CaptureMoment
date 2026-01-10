/**
 * @file i_xmp_path_strategy.h
 * @brief Declaration of IXmpPathStrategy interface
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <string>

namespace CaptureMoment::Core {

namespace Serializer {

/**
 * @brief Interface defining the strategy for determining XMP file paths based on image paths.
 * This allows flexible storage locations for XMP sidecar files (e.g., alongside image, in app data, configurable).
 */
class IXmpPathStrategy {
public:
    virtual ~IXmpPathStrategy() = default;

    /**
     * @brief Calculates the XMP file path associated with a given source image path.
     * @param source_image_path The path to the source image file.
     * @return The calculated path where the corresponding XMP file should be located.
     */
    [[nodiscard]] virtual std::string getXmpPathForImage(std::string_view source_image_path) const = 0;

    /**
     * @brief Calculates the source image path associated with a given XMP file path.
     * This is the inverse of getXmpPathForImage.
     * @param xmp_path The path to the XMP file.
     * @return The calculated path of the source image file, or an empty string if it cannot be determined.
     */
    [[nodiscard]] virtual std::string getImagePathFromXmp(std::string_view xmp_path) const = 0;
};

} // namespace Serializer

} // namespace CaptureMoment::Core
