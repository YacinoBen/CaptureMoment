/**
 * @file source_manager.h
 * @brief Image source management class using OpenImageIO (OIIO).
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include "managers/i_source_manager.h"
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagecache.h>
#include <memory>
#include <string_view>
#include <optional>

namespace CaptureMoment::Core {

namespace Managers {
/**
 * @class SourceManager
 * @brief Concrete implementation of the ISourceManager interface.
 * * This class is responsible for loading, unloading, and accessing 
 * source image data using the OpenImageIO library tools.
 * * It manages the main image buffer (ImageBuf) and uses a global cache 
 * (ImageCache) to optimize read access and memory management during tile-based 
 * processing.
 * * @note The SourceManager manages the lifetime of the OIIO image buffer via 
 * a std::unique_ptr, ensuring proper resource release (RAII).
 * * @see ISourceManager
 */
class SourceManager : public ISourceManager {
public:
    /**
     * @brief Default constructor.
     * * Initializes the pointer to the global OIIO cache.
     */
    SourceManager();

    /**
     * @brief Destructor.
     * * Ensures clean release of image resources.
     */
    ~SourceManager() override;
    
    // -----------------------------------------------------------------
    // ISourceManager Implementation
    // -----------------------------------------------------------------
    [[nodiscard]] bool loadFile(std::string_view path) override;
    void unload() override;
    [[nodiscard]] bool isLoaded() const override;
    [[nodiscard]] int width() const noexcept override;
    [[nodiscard]] int height() const noexcept override;
    [[nodiscard]] int channels() const noexcept override;
    
    std::unique_ptr<Common::ImageRegion> getTile(
        int x, int y, int width, int height
    ) override;

    [[nodiscard]] bool setTile(const Common::ImageRegion& tile) override;
    
    [[nodiscard]] std::optional<std::string> getMetadata(std::string_view key) const override;
    
private:
    /**
     * @brief The main image buffer containing the loaded image data.
     * * Managed by a std::unique_ptr for RAII compliance.
     */
    std::unique_ptr<OIIO::ImageBuf> m_image_buf;

    /**
     * @brief Pointer to OIIO's global image cache.
     * * Used for tile management and resource handling. This class does NOT 
     * manage the lifetime of the ImageCache object.
     */
    OIIO::ImageCache* m_cache; 

    /**
     * @brief The file path of the currently loaded image.
     */
    std::string m_current_path;
    
    /**
     * @brief Provides access to OIIO's global ImageCache singleton.
     * @return Pointer to the global ImageCache.
     */
    static OIIO::ImageCache* getGlobalCache();
};

} // namespace Managers

} // namespace CaptureMoment::Core

