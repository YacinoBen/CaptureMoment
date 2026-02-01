/**
 * @file i_source_manager.h
 * @brief Interface for managing image sources
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include "common/image_region.h"

#include <string>
#include <string_view>
#include <memory>
#include <optional>

namespace CaptureMoment::Core {

namespace Managers {
/**
 * @brief Abstract interface for managing image sources.
 *
 * Defines the basic operations for loading, unloading, inspecting, and extracting
 * regions (tiles) from image sources. This interface allows for easy swapping
 * of different underlying image source management implementations (e.g., OIIO).
 *
 * @note All methods are pure virtual, ensuring a complete contract for implementers.
 *       Methods marked `noexcept` do not throw exceptions under normal circumstances.
 */
class ISourceManager {
public:
    /**
     * @brief Virtual destructor.
    */
    virtual ~ISourceManager() = default;

    /**
     * @brief Loads an image file from the specified path.
     *
     * This method loads the image and converts it internally to RGBA_F32 (4 channels, 32-bit float).
     * This pre-conversion optimizes subsequent `getTile()` calls.
     *
     * @param path The file system path to the image.
     * @return true if the file was loaded and converted successfully, false otherwise.
     */
    [[nodiscard]] virtual bool loadFile(std::string_view path) = 0;

    /**
    * @brief Unloads the currently loaded image and frees resources.
    */
    virtual void unload() = 0;
    

    /**
    * @brief Checks if an image is currently loaded.
    * @return true if an image is loaded, false otherwise.
    */
   [[nodiscard]] virtual bool isLoaded() const = 0;

    /**
     * @brief Retrieves the width (in pixels) of the source image.
     * @pre isLoaded() must be true.
     * @return The image width, or 0 if no image is loaded.
     */
    [[nodiscard]] virtual int width() const noexcept = 0;

    /**
     * @brief Retrieves the height (in pixels) of the source image.
     * @pre isLoaded() must be true.
     * @return The image height, or 0 if no image is loaded.
     */
    [[nodiscard]] virtual int height() const noexcept = 0;

    /**
     * @brief Gets the number of channels of the internal buffer.
     * @note Always returns 4 because `loadFile` forces RGBA conversion.
     * @return Number of channels (4), or 0 if not loaded.
     */
   [[nodiscard]] virtual int channels() const noexcept = 0;
    
    /**
     * @brief Extracts a rectangular region (tile) of pixels from the image.
     *
     * This method is thread-safe. It performs the following logic:
     * 1. Clamps the requested coordinates to the valid image boundaries.
     * 2. Allocates a new buffer for the tile data.
     * 3. Copies pixel data in RGBA_F32 format into the buffer.
     *
     * @param x X-coordinate of the top-left corner of the requested region.
     * @param y Y-coordinate of the top-left corner of the requested region.
     * @param width Width of the requested region.
     * @param height Height of the requested region.
     * @return A unique pointer to an ImageRegion containing the pixel data, or nullptr on error.
     */
    [[nodiscard]] virtual std::unique_ptr<Common::ImageRegion> getTile(
        int x, int y, int width, int height
    ) = 0;

    /**
     * @brief Writes pixel data from a tile back into the image buffer.
     *
     * This method is thread-safe. It validates the tile format and bounds
     * before updating the internal image buffer.
     *
     * @param tile The ImageRegion containing the pixel data to write.
     * @return true if the tile was written successfully, false on error (bounds/format mismatch).
     */
    [[nodiscard]] virtual bool setTile(const Common::ImageRegion& tile) = 0;

    /**
     * @brief Retrieves a specific metadata field from the source image.
     * @param key Name of the metadata field to search for (e.g., "Make", "DateTime", etc.).
     * @return The metadata value as a string. Returns an empty string if the key is not found.
     */
    [[nodiscard]] virtual std::optional<std::string> getMetadata(std::string_view key) const = 0;
};

} // namespace Managers

} // namespace CaptureMoment::core
