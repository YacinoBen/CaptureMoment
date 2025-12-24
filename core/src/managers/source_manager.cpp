/**
 * @file source_manager.cpp
 * @brief Image source management class using OpenImageIO (OIIO).
 * @author CaptureMoment Team
 * @date 2025
 */

#include <spdlog/spdlog.h>

#include "managers/source_manager.h"
#include <algorithm>

namespace CaptureMoment::Core::Managers {

static std::shared_ptr<OIIO::ImageCache> s_globalCachePtr = nullptr;

OIIO::ImageCache* SourceManager::getGlobalCache()
{
    if (!s_globalCachePtr) {
        // create the global ImageCache singleton
        s_globalCachePtr = OIIO::ImageCache::create();
        
        // Configuration with the raw pointer obtained via get()
        OIIO::ImageCache* cache = s_globalCachePtr.get();
        if (cache) {
            cache->attribute("max_memory_MB", 2048.0f);
            spdlog::debug("OIIO ImageCache created: 2GB");
        } else {
             // Should be impossible if create() succeeds
            spdlog::critical("Failed to create OIIO ImageCache.");
        }
    }
    // Return the raw pointer for access by other classes
    return s_globalCachePtr.get();
}

SourceManager::SourceManager()
    : m_cache(getGlobalCache())
{
    if (!m_cache) {
        spdlog::critical("SourceManager: Failed to get global ImageCache, initialization failed.");
        throw std::runtime_error("SourceManager: Failed to get global ImageCache.");
    }
    spdlog::debug("SourceManager: Instance created.");
}

SourceManager::~SourceManager()
{
    unload();
    spdlog::debug("SourceManager: Instance destroyed.");
}

bool SourceManager::loadFile(std::string_view path)
{

    if (path.empty()) {
        spdlog::warn("SourceManager::loadFile: Empty file path provided.");
        return false;
    }

    // 1. If an image is already loaded, unload it first (cleanup old state)
    if (isLoaded()) {
        unload(); // Calls m_image_buf.reset() internally
    }

    spdlog::info("SourceManager: Attempting to load file: '{}'", path);

    try {
        m_image_buf = std::make_unique<OIIO::ImageBuf>(
            std::string(path),
            0, 0,  // subimage, miplevel
            s_globalCachePtr
        );
        
        if (!m_image_buf->read())
        {
            spdlog::warn("SourceManager: Failed to read file '{}'. OIIO Message: {}", 
                         path, m_image_buf->geterror());
            m_image_buf.reset();
            return false;
        }
        
        m_current_path = path;

        // LOG: Success log with dimensions
        spdlog::info("SourceManager: Successfully loaded '{}'. Resolution: {}x{} ({} channels).", 
                     m_current_path, width(), height(), channels());
        return true;
        
    } catch (const std::exception& e) {
        spdlog::critical("SourceManager: C++ Exception during file loading of '{}': {}", 
                         path, e.what());
        
        m_image_buf.reset();
        return false;
    }
}

void SourceManager::unload()
{

    if (isLoaded()) {
    spdlog::info("SourceManager: Unloading '{}'.", m_current_path);
    }

    m_image_buf.reset();
    m_current_path.clear();
}

bool SourceManager::isLoaded() const {
    return m_image_buf && m_image_buf->initialized();
}

int SourceManager::width() const noexcept {
    return isLoaded() ? m_image_buf->spec().width : 0;
}

int SourceManager::height() const noexcept {
    return isLoaded() ? m_image_buf->spec().height : 0;
}

int SourceManager::channels() const noexcept {
    return isLoaded() ? m_image_buf->spec().nchannels : 0;
}

std::unique_ptr<Common::ImageRegion> SourceManager::getTile(
    int x, int y, int width, int height) 
{
    if (!isLoaded()) {
        spdlog::warn("getTile() called but no image loaded");
        return nullptr;
    }


    if (width <= 0 || height <= 0) {
        spdlog::warn("SourceManager::getTile: Invalid dimensions requested ({}x{})", width, height);
        return nullptr;
    }
    
    // 1. Clamping coordinates to stay within bounds
    // Note: OIIO ROI uses exclusive max coordinates, but we clamp the requested size here.

    int clamped_x = std::clamp(x, 0, this->width() - 1);
    int clamped_y = std::clamp(y, 0, this->height() -1);
    int clamped_width = std::min(width, this->width() - clamped_x);
    int clamped_height = std::min(height, this->height() - clamped_y);
    

    if (clamped_width <= 0 || clamped_height <= 0) {
        spdlog::warn("SourceManager::getTile: Clamped region has zero area ({}x{})", clamped_width, clamped_height);
        return nullptr;
    }

    spdlog::info("SourceManager::getTile : clamped_x : {}, clamped_y : {}, clamped_width : {}, clamped_height: {}", clamped_x,clamped_y, clamped_width, clamped_height);
     // 2. Preparation of ImageRegion
    auto region = std::make_unique<Common::ImageRegion>();
    region->m_x = clamped_x;
    region->m_y = clamped_y;
    region->m_width = clamped_width;
    region->m_height = clamped_height;
    region->m_channels = 4;  // Force RGBA
    region->m_format = Common::PixelFormat::RGBA_F32;

    // Allocation of the memory buffer (4 channels * sizeof(float))
    // We resize the vector to hold the required bytes.
    const size_t dataSize = static_cast<size_t>(clamped_width * clamped_height * 4);
    region->m_data.resize(dataSize);
    
    int source_channels = this->channels();

      if (source_channels == 4)
      {
        OIIO::ROI roi(
            clamped_x, clamped_x + clamped_width,
            clamped_y, clamped_y + clamped_height,
            0, 1,  // Z
            0, 4   // Channels
        );

        if (!m_image_buf->get_pixels(roi, OIIO::TypeDesc::FLOAT, region->m_data.data())) {
            spdlog::warn("Failed to extract RGBA tile at ({}, {})", clamped_x, clamped_y);
            return nullptr;
        }

        spdlog::info("SourceManager::getTile: Read 4-channel data directly.");

      } else if (source_channels == 3) {
        std::vector<float> tempRGB(clamped_width * clamped_height * 3);

        OIIO::ROI roi(
            clamped_x, clamped_x + clamped_width,
            clamped_y, clamped_y + clamped_height,
            0, 1, // Z
            0, 3   
        );
        
        if (!m_image_buf->get_pixels(roi, OIIO::TypeDesc::FLOAT, tempRGB.data())) {
            spdlog::warn("Failed to extract RGB tile at ({}, {})", clamped_x, clamped_y);
            return nullptr;
        }

        for (int i = 0; i < clamped_width * clamped_height; ++i) 
        {
            region->m_data[i * 4 + 0] = tempRGB[i * 3 + 0]; // R
            region->m_data[i * 4 + 1] = tempRGB[i * 3 + 1]; // G
            region->m_data[i * 4 + 2] = tempRGB[i * 3 + 2]; // B
            region->m_data[i * 4 + 3] = 1.0f;                // ✅ Alpha
        }

        spdlog::info("SourceManager::getTile: Converted 3-channel data to 4-channel.");
    }
    else if (source_channels == 1)
    {
        // Grayscale source
        std::vector<float> tempGray(clamped_width * clamped_height);

        OIIO::ROI roi(
            clamped_x, clamped_x + clamped_width,
            clamped_y, clamped_y + clamped_height,
            0, 1,  // Z
            0, 1
        );
        
        if (!m_image_buf->get_pixels(roi, OIIO::TypeDesc::FLOAT, tempGray.data())) {
            spdlog::warn("Failed to extract grayscale tile at ({}, {})", clamped_x, clamped_y);
            return nullptr;
        }

        // Conversion Grayscale → RGBA
        for (int i = 0; i < clamped_width * clamped_height; ++i)
        {
            float gray = tempGray[i];
            region->m_data[i * 4 + 0] = gray; // R
            region->m_data[i * 4 + 1] = gray; // G
            region->m_data[i * 4 + 2] = gray; // B
            region->m_data[i * 4 + 3] = 1.0f; // Alpha
        }
        spdlog::info("SourceManager::getTile: Converted 1-channel data to 4-channel.");

    } else {
        spdlog::error("Unsupported source channel count: {}", source_channels);
        return nullptr;
    }

    if (region->m_data.size() != static_cast<size_t>(region->m_width) * region->m_height * region->m_channels) {
        spdlog::critical("SourceManager::getTile: Inconsistency! data.size()={}, calculated size={}", region->m_data.size(), static_cast<size_t>(region->m_width) * region->m_height * region->m_channels);
        return nullptr; // Ou assertion
    }

    spdlog::info("SourceManager: Tile extracted: ({}, {}) {}*{} → RGBA_F32 (from {} channels source)",
                  clamped_x, clamped_y, clamped_width, clamped_height, source_channels);

    return region;
}

bool SourceManager::setTile(const Common::ImageRegion& tile)
{
    if (!isLoaded()) {
        spdlog::warn("SourceManager::setTile: Called but no image loaded");
        return false;
    }

    if (!tile.isValid()) {
        spdlog::error("SourceManager::setTile: Received an invalid ImageRegion.");
        return false;
    }

    if (tile.m_format != Common::PixelFormat::RGBA_F32) {
        spdlog::error("SourceManager::setTile: ImageRegion format is not RGBA_F32 (got {}).", static_cast<int>(tile.m_format));
        return false;
    }

    if (tile.m_channels != 4) {
        spdlog::error("SourceManager::setTile: ImageRegion does not have 4 channels (got {}).", tile.m_channels);
        return false;
    }

    // Vérification des bornes
    if (tile.m_x < 0 || tile.m_y < 0 ||
        tile.m_x + tile.m_width > this->width() ||
        tile.m_y + tile.m_height > this->height()) {
        spdlog::error("SourceManager::setTile: Tile coordinates ({},{}) size ({}x{}) are out of image bounds ({}x{}).",
                      tile.m_x, tile.m_y, tile.m_width, tile.m_height, this->width(), this->height());
        return false;
    }

    // 1. Definition of the OIIO ROI for writing
    OIIO::ROI roi(
        tile.m_x, tile.m_x + tile.m_width,
        tile.m_y, tile.m_y + tile.m_height,
        0, 1, // Zmin, Zmax
        0, 4 // Cmin, Cmax (4 channels for writing)
        );

    // 2. Writing pixels back into the ImageBuf
    // The ImageBuf is updated using TypeDesc::FLOAT (corresponding to std::vector<float>).
    if (!m_image_buf->set_pixels(roi, OIIO::TypeDesc::FLOAT, tile.m_data.data())) {
        spdlog::error("SourceManager::setTile: Failed to write tile back at ({}, {}). OIIO Error: {}",
                      tile.m_x, tile.m_y, m_image_buf->geterror());
        return false;
    }

    spdlog::trace("SourceManager::setTile: Tile written back: ({}, {}) {}x{} (RGBA_F32).",
                  tile.m_x, tile.m_y, tile.m_width, tile.m_height);
    return true;
}

std::optional<std::string> SourceManager::getMetadata(std::string_view key) const 
{
    if (!isLoaded()) {
        return std::nullopt;
    }
    
    const auto& spec = m_image_buf->spec();
    auto* attr = spec.find_attribute(std::string(key));

   // to avoid problematic implicit conversion of the ternary operator.
    if (attr) {
        return attr->get_string();
    } else {
        // Retourne std::nullopt si l'attribut n'est pas trouvé
        return std::nullopt;
    }
}

} // namespace CaptureMoment::Core::Managers
