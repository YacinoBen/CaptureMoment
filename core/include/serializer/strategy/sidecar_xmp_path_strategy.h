// core/include/serializer/sidecar_xmp_path_strategy.h
#pragma once

#include "serializer/strategy/i_xmp_path_strategy.h"
#include <string>

namespace CaptureMoment::Core::Serializer {

/**
 * @brief Implementation of IXmpPathStrategy that stores XMP files alongside the image file.
 * The XMP file shares the same name as the image file but with an added ".xmp" extension.
 * For example, "/home/user/pics/holiday.jpg" maps to "/home/user/pics/holiday.jpg.xmp".
 */
class SidecarXmpPathStrategy final : public IXmpPathStrategy {
public:
    SidecarXmpPathStrategy() = default;
    ~SidecarXmpPathStrategy() override = default;

    // Delete copy/move if not needed for this simple strategy
    SidecarXmpPathStrategy(const SidecarXmpPathStrategy&) = delete;
    SidecarXmpPathStrategy& operator=(const SidecarXmpPathStrategy&) = delete;
    SidecarXmpPathStrategy(SidecarXmpPathStrategy&&) = delete;
    SidecarXmpPathStrategy& operator=(SidecarXmpPathStrategy&&) = delete;

    /**
     * @brief Calculates the XMP file path associated with a given source image path.
     * The XMP file is placed in the same directory as the image with ".xmp" appended to the full name.
     * @param source_image_path The path to the source image file.
     * @return The calculated path where the corresponding XMP file should be located.
     */
    [[nodiscard]] std::string getXmpPathForImage(std::string_view source_image_path) const override;

    /**
     * @brief Calculates the source image path associated with a given XMP file path.
     * This is the inverse of getXmpPathForImage. It removes the ".xmp" extension.
     * @param xmp_path The path to the XMP file.
     * @return The calculated path of the source image file, or an empty string if it cannot be determined.
     */
    [[nodiscard]] std::string getImagePathFromXmp(std::string_view xmp_path) const override;
};

} // namespace CaptureMoment::Core::Serializer
