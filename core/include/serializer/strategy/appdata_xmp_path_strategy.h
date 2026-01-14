/**
 * @file appdata_xmp_path_strategy.h
 * @brief Declaration of AppDataXmpPathStrategy class
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include "serializer/strategy/i_xmp_path_strategy.h"
#include "serializer/provider/i_xmp_provider.h"
#include <memory>
#include <string>

namespace CaptureMoment::Core::Serializer {

/**
 * @brief Implementation of IXmpPathStrategy that stores XMP files in a centralized AppData directory.
 * The structure of the image's path relative to its root is mirrored in the AppData directory.
 * For example, "/home/user/pics/holiday.jpg" might map to "<app_data_dir>/pics/holiday.jpg.xmp".
 */
class AppDataXmpPathStrategy final : public IXmpPathStrategy {
public:
    /**
     * @brief Constructs an AppDataXmpPathStrategy.
     * @param app_data_dir The base directory where XMP files will be stored (e.g., ~/.local/share/CaptureMoment/xmp_cache/).
     */
    explicit AppDataXmpPathStrategy(const std::string& app_data_dir);

    ~AppDataXmpPathStrategy() override = default;

    // Delete copy/move for simplicity, assuming m_app_data_dir is const after construction
    AppDataXmpPathStrategy(const AppDataXmpPathStrategy&) = delete;
    AppDataXmpPathStrategy& operator=(const AppDataXmpPathStrategy&) = delete;
    AppDataXmpPathStrategy(AppDataXmpPathStrategy&&) = delete;
    AppDataXmpPathStrategy& operator=(AppDataXmpPathStrategy&&) = delete;

    /**
     * @brief Calculates the XMP file path associated with a given source image path.
     * The path is constructed within the configured AppData directory, mirroring the relative path structure.
     * @param source_image_path The path to the source image file.
     * @return The calculated path where the corresponding XMP file should be located.
     */
    [[nodiscard]] std::string getXmpPathForImage(std::string_view source_image_path) const override;

    /**
     * @brief Calculates the source image path associated with a given XMP file path.
     * This is the inverse of getXmpPathForImage. It's complex to implement reliably without storing
     * the original path in the XMP itself, so this implementation currently logs a warning and returns empty.
     * @param xmp_path The path to the XMP file.
     * @return The calculated path of the source image file, or an empty string if it cannot be determined.
     */
    [[nodiscard]] std::string getImagePathFromXmp(std::string_view xmp_path) const override;

private:
    /**
     * @brief The base AppData directory.
     */
    const std::string m_app_data_dir;

    /**
     * @brief Provider to read XMP packets.
     */
    std::unique_ptr<IXmpProvider> m_xmp_provider;

};

} // namespace CaptureMoment::Core::Serializer
