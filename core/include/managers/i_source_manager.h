/**
 * @file i_source_manager.h
 * @brief Interface for managing image sources
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once
#include "common/types.h"

#include <string>
#include <memory>
#include <optional>
#include "common/image_region.h"

namespace CaptureMoment {

// Forward declaration to avoid full dependency on ImageRegion in the interface
struct ImageRegion; 

/**
 * @brief interface for for managing image sources
 * * Defines the basic operations for loading, unloading, inspecting, and extracting
 * regiones (tiles) from image sources. This interface is intended to be implemented.
 * This interface allows for easy swapping of different image source management of  underlying implementations.
 * (e.g., OIIO, etc).
 */

class ISourceManager {
public:
    /**
     * @brief Virtual destructor.
    */
    virtual ~ISourceManager() = default;
    
    /**
     * @brief Loads an image from the specified path.
     * @param path Path to the image file (e.g., .exr, .tif, .jpg, etc.).
     * @return true if loading is successful and the file is valid, false otherwise.
     */
    virtual bool loadFile(std::string_view path) = 0;

    /**
    * @brief Unloads the currently loaded image and frees resources.
    */
    virtual void unload() = 0;
    

    /**
    * @brief Checks if an image is currently loaded.
    * @return true if an image is loaded, false otherwise.
    */
    virtual bool isLoaded() const = 0;

    /**
     * @brief Retrieves the width (in pixels) of the source image.
     * @pre isLoaded() must be true.
     * @return The image width, or 0 if no image is loaded.
     */
    virtual int width() const = 0;

        

    /**
     * @brief Retrieves the height (in pixels) of the source image.
     * @pre isLoaded() must be true.
     * @return The image height, or 0 if no image is loaded.
     */
    virtual int height() const = 0;

        
    /**
     * @brief Retrieves the number of channels in the source image (e.g., 3 for RGB, 4 for RGBA).
     * @pre isLoaded() must be true.
     * @return The number of channels.
     */
    virtual int channels() const = 0;
    
        
    /**
     * @brief Extracts a specific tile (region) from the source image.
     * * This method is critical for tile-based processing. It copies the 
     * data of the specified region into a new ImageRegion object.
     * * @param x Starting X-coordinate of the tile.
     * @param y Starting Y-coordinate of the tile.
     * @param width Width of the tile to extract.
     * @param height Height of the tile to extract.
     * @return A unique pointer to a new ImageRegion containing the data, 
     * or nullptr on failure (e.g., tile out of bounds).
     */
    virtual std::unique_ptr<ImageRegion> getTile(
        int x, int y, int width, int height
    ) = 0;
    
    /**
     * @brief Retrieves a specific metadata field from the source image.
     * @param key Name of the metadata field to search for (e.g., "Make", "DateTime", etc.).
     * @return The metadata value as a string. Returns an empty string if the key is not found.
     */
    virtual std::optional<std::string> getMetadata(std::string_view key) const = 0;
};

} // namespace CaptureMoment