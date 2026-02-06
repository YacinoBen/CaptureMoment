/**
 * @file configurable_xmp_path_strategy.h
 * @brief Declaration of ConfigurableXmpPathStrategy class
 * @details Strategy to store XMP files in a user-configured directory mirroring relative structure.
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include "serializer/provider/i_xmp_provider.h"
#include "serializer/strategy/i_xmp_path_strategy.h"

#include <string_view>
#include <memory>

namespace CaptureMoment::Core::Serializer {

/**
 * @brief Implementation of IXmpPathStrategy that stores XMP files in a user-configured directory.
 *
 * The structure of the image's path relative to its root is mirrored in the configured directory.
 * For example, with config "/home/user/custom_xmp/", "/home/user/pics/holiday.jpg" might map to
 * "/home/user/custom_xmp/pics/holiday.jpg.xmp".
 *
 * @note This strategy assumes the provided source image path is absolute to correctly resolve the root path.
 */
class ConfigurableXmpPathStrategy final : public IXmpPathStrategy {
public:
    /**
     * @brief Constructs a ConfigurableXmpPathStrategy.
     *
     * @param base_xmp_dir The base directory configured by the user where XMP files will be stored.
     * @param xmp_provider A unique pointer to an XMP provider for reading XMP packets. Must not be null.
     */
    explicit ConfigurableXmpPathStrategy(const std::string& base_xmp_dir, std::unique_ptr<IXmpProvider> xmp_provider);

    ~ConfigurableXmpPathStrategy() override = default;

    // Delete copy/move
    ConfigurableXmpPathStrategy(const ConfigurableXmpPathStrategy&) = delete;
    ConfigurableXmpPathStrategy& operator=(const ConfigurableXmpPathStrategy&) = delete;
    ConfigurableXmpPathStrategy(ConfigurableXmpPathStrategy&&) = delete;
    ConfigurableXmpPathStrategy& operator=(ConfigurableXmpPathStrategy&&) = delete;

    /**
     * @brief Calculates the XMP file path associated with a given source image path.
     *
     * The path is constructed within the configured base directory, potentially mirroring the relative path structure.
     *
     * @param source_image_path The path to the source image file.
     * @return The calculated path where the corresponding XMP file should be located.
     */
    [[nodiscard]] std::string getXmpPathForImage(std::string_view source_image_path) const override;

    /**
     * @brief Calculates the source image path associated with a given XMP file path.
     *
     * This is the inverse of `getXmpPathForImage`. It reads the XMP file to extract the original image path
     * stored as metadata (`Xmp.cm.sourceImagePath`).
     *
     * @param xmp_path The path to the XMP file.
     * @return The calculated path of the source image file, or an empty string if it cannot be determined
     *         (e.g., file not found, no XMP packet, no sourceImagePath tag).
     */
    [[nodiscard]] std::string getImagePathFromXmp(std::string_view xmp_path) const override;

private:
    /**
     * @brief The base directory configured by the user.
     */
    const std::string m_base_xmp_dir;

    /**
     * @brief Provider to read XMP packets.
     */
    std::unique_ptr<IXmpProvider> m_xmp_provider;
};

} // namespace CaptureMoment::Core::Serializer
