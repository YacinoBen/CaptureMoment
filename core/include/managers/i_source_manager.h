/**
 * @file i_source_manager.h
 * @brief Abstract interface for managing image sources.
 *
 * @details
 * Defines the contract for loading, unloading, and accessing image data.
 * Implementations are responsible for thread safety, tile access, and
 * potential internal caching strategies.
 *
 * All methods returning `std::expected` are exception-free regarding the
 * return types they handle.
 *
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include "common/image_region.h"
#include "common/error_handling/core_error.h"

#include <string_view>
#include <memory>
#include <optional>
#include <expected>

namespace CaptureMoment::Core {

namespace Managers {

/**
 * @brief Abstract interface for managing image sources.
 *
 * Defines the basic operations for loading, unloading, inspecting, and extracting
 * regions (tiles) from image sources. This interface allows for easy swapping
 * of different underlying image source management implementations (e.g., OIIO).
 */
class ISourceManager {
public:
    virtual ~ISourceManager() = default;

    /**
     * @brief Loads an image file from the specified path.
     *
     * Implementations should handle file system access and initial parsing.
     * For optimization, implementations are encouraged to convert the source
     * format to the internal standard (RGBA_F32) during this call.
     *
     * @param path The file system path to the image.
     * @return `std::expected<void, CoreError>`:
     *         - Returns void on success.
     *         - Returns CoreError on failure (FileNotFound, DecodingError, etc.).
     */
    [[nodiscard]] virtual std::expected<void, ErrorHandling::CoreError> loadFile(std::string_view path) = 0;

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
     * @return The image width, or 0 if no image is loaded.
     */
    [[nodiscard]] virtual int width() const noexcept = 0;

    /**
     * @brief Retrieves the height (in pixels) of the source image.
     * @return The image height, or 0 if no image is loaded.
     */
    [[nodiscard]] virtual int height() const noexcept = 0;

    /**
     * @brief Gets the number of channels of the internal buffer.
     * @return Number of channels (typically 4 for RGBA), or 0 if not loaded.
     */
    [[nodiscard]] virtual int channels() const noexcept = 0;

    /**
     * @brief Extracts a rectangular region (tile) of pixels from the image.
     *
     * Implementations must be thread-safe.
     * The requested region should be clamped to valid image boundaries.
     *
     * @param x X-coordinate of the top-left corner.
     * @param y Y-coordinate of the top-left corner.
     * @param width Width of the requested region.
     * @param height Height of the requested region.
     * @return `std::expected<std::unique_ptr<Common::ImageRegion>, CoreError>`:
     *         - Unique pointer to the extracted tile on success.
     *         - CoreError (e.g., InvalidImageRegion) if extraction fails.
     */
    [[nodiscard]] virtual std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError> getTile(
        int x, int y, int width, int height
        ) = 0;

    /**
     * @brief Writes pixel data from a tile back into the image buffer.
     *
     * Implementations must be thread-safe and validate tile format and bounds.
     *
     * @param tile The ImageRegion containing the pixel data to write.
     * @return `std::expected<void, CoreError>`:
     *         - Returns void on success.
     *         - CoreError on failure (IOError, InvalidImageRegion, etc.).
     */
    [[nodiscard]] virtual std::expected<void, ErrorHandling::CoreError> setTile(const Common::ImageRegion& tile) = 0;

    /**
     * @brief Retrieves a specific metadata field from the source image.
     * @param key Name of the metadata field to search for.
     * @return std::optional<string> containing the value, or std::nullopt if not found.
     */
    [[nodiscard]] virtual std::optional<std::string> getMetadata(std::string_view key) const = 0;
};

} // namespace Managers

} // namespace CaptureMoment::Core
