/**
 * @file source_manager.cpp
 * @brief Image source management class using OpenImageIO (OIIO).
 * @author CaptureMoment Team
 * @date 2025
 */

#include <spdlog/spdlog.h>

#include "managers/source_manager.h"
#include <algorithm>

namespace CaptureMoment {

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
    spdlog::debug("SourceManager: Instance created.");
}

SourceManager::~SourceManager()
{
    unload();
    spdlog::debug("SourceManager: Instance destroyed.");
}

bool SourceManager::loadFile(std::string_view path)
{

    // 1. If an image is already loaded, unload it first (cleanup old state)
    if (isLoaded()) {
        unload(); // Calls m_imageBuf.reset() internally
    }

    spdlog::info("SourceManager: Attempting to load file: '{}'", path);

    try {
        m_imageBuf = std::make_unique<OIIO::ImageBuf>(
            std::string(path),
            0, 0,  // subimage, miplevel
            s_globalCachePtr
        );
        
        if (!m_imageBuf->read()) 
        {
            spdlog::warn("SourceManager: Failed to read file '{}'. OIIO Message: {}", 
                         path, m_imageBuf->geterror());
            m_imageBuf.reset();
            return false;
        }
        
        m_currentPath = path;

        // LOG: Success log with dimensions
        spdlog::info("SourceManager: Successfully loaded '{}'. Resolution: {}x{} ({} channels).", 
                     m_currentPath, width(), height(), channels());
        return true;
        
    } catch (const std::exception& e) {
        spdlog::critical("SourceManager: C++ Exception during file loading of '{}': {}", 
                         path, e.what());
        
        m_imageBuf.reset();
        return false;
    }
}

void SourceManager::unload()
{

    if (isLoaded()) {
    spdlog::info("SourceManager: Unloading '{}'.", m_currentPath);
    }

    m_imageBuf.reset();
    m_currentPath.clear();
}

bool SourceManager::isLoaded() const {
    return m_imageBuf && m_imageBuf->initialized();
}

int SourceManager::width() const noexcept {
    return isLoaded() ? m_imageBuf->spec().width : 0;
}

int SourceManager::height() const noexcept {
    return isLoaded() ? m_imageBuf->spec().height : 0;
}

int SourceManager::channels() const noexcept {
    return isLoaded() ? m_imageBuf->spec().nchannels : 0;
}

std::unique_ptr<ImageRegion> SourceManager::getTile(
    int x, int y, int width, int height) 
{
    if (!isLoaded()) {
        spdlog::warn("getTile() called but no image loaded");
        return nullptr;
    }
    
    // 1. Clamping coordinates to stay within bounds
    // Note: OIIO ROI uses exclusive max coordinates, but we clamp the requested size here.

    int clamped_x = std::clamp(x, 0, this->width() - 1);
    int clamped_y = std::clamp(y, 0, this->height() -1);
    int clamped_width = std::min(width, this->width() - clamped_x);
    int clamped_height = std::min(height, this->height() - clamped_y);
    
    spdlog::info("SourceManager::getTile : clamped_x : {}, clamped_y : {}, clamped_width : {}, clamped_height: {}", clamped_x,clamped_y, clamped_width, clamped_height);
     // 2. Preparation of ImageRegion
    auto region = std::make_unique<ImageRegion>();
    region->m_x = clamped_x;
    region->m_y = clamped_y;
    region->m_width = clamped_width;
    region->m_height = clamped_height;
    region->m_channels = 4;  // Force RGBA
    region->m_format = PixelFormat::RGBA_F32;

    // Allocation of the memory buffer (4 channels * sizeof(float))
    // We resize the vector to hold the required bytes.
    const size_t dataSize = static_cast<size_t>(clamped_width * clamped_height * 4);
    region->m_data.resize(dataSize);
    
    int sourceChannels = this->channels();

      if (sourceChannels == 4) 
      {
        OIIO::ROI roi(
            clamped_x, clamped_x + clamped_width,
            clamped_y, clamped_y + clamped_height,
            0, 1,  // Z
            0, 4   // Channels
        );

        if (!m_imageBuf->get_pixels(roi, OIIO::TypeDesc::FLOAT, region->m_data.data())) {
            spdlog::warn("Failed to extract RGBA tile at ({}, {})", clamped_x, clamped_y);
            return nullptr;
        }

        spdlog::info("SourceManager::getTile: Read 4-channel data directly.");

      } else if (sourceChannels == 3) {
        std::vector<float> tempRGB(clamped_width * clamped_height * 3);

        OIIO::ROI roi(
            clamped_x, clamped_x + clamped_width,
            clamped_y, clamped_y + clamped_height,
            0, 1, // Z
            0, 3   
        );
        
        if (!m_imageBuf->get_pixels(roi, OIIO::TypeDesc::FLOAT, tempRGB.data())) {
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
    else if (sourceChannels == 1) 
    {
        // Grayscale source
        std::vector<float> tempGray(clamped_width * clamped_height);

        OIIO::ROI roi(
            clamped_x, clamped_x + clamped_width,
            clamped_y, clamped_y + clamped_height,
            0, 1,  // Z
            0, 1   // ✅ Lire seulement 1 canal
        );
        
        if (!m_imageBuf->get_pixels(roi, OIIO::TypeDesc::FLOAT, tempGray.data())) {
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
        spdlog::error("Unsupported source channel count: {}", sourceChannels);
        return nullptr;
    }

    if (region->m_data.size() != static_cast<size_t>(region->m_width) * region->m_height * region->m_channels) {
        spdlog::critical("SourceManager::getTile: Inconsistency! data.size()={}, calculated size={}", region->m_data.size(), static_cast<size_t>(region->m_width) * region->m_height * region->m_channels);
        return nullptr; // Ou assertion
    }

    spdlog::info("SourceManager: Tile extracted: ({}, {}) {}*{} → RGBA_F32 (from {} channels source)",
                  clamped_x, clamped_y, clamped_width, clamped_height, sourceChannels);

    return region;
}

bool SourceManager::setTile(const ImageRegion& tile)
{
    if (!isLoaded()) {
        spdlog::warn("setTile() called but no image loaded");
        return false;
    }

    if (!tile.isValid() || tile.m_format != PixelFormat::RGBA_F32 || tile.m_channels != 4) {
        spdlog::error("setTile() received an invalid or unsupported ImageRegion (must be RGBA_F32, 4 channels).");
        return false;
    }

    // 1. Definition of the OIIO ROI for writing
    OIIO::ROI roi(
        tile.m_x, tile.m_x + tile.m_width,
        tile.m_y, tile.m_y + tile.m_height,
        0, 1, // Zmin, Zmax
        0, 4 // Cmin, Cmax (4 channels for writing)
    );

    // 2. writing pixels back into the ImageBuf
    // The ImageBuf is updated using TypeDesc::FLOAT (corresponding to std::vector<float>).
    if (!m_imageBuf->set_pixels(roi, OIIO::TypeDesc::FLOAT, tile.m_data.data())) {
        spdlog::error("Failed to write tile back at ({}, {}). OIIO Error: {}",
                      tile.m_x, tile.m_y, m_imageBuf->geterror());
        return false;
    }

    spdlog::trace("SourceManager: Tile written back: ({}, {}) {}x{} (RGBA_F32).",
                  tile.m_x, tile.m_y, tile.m_width, tile.m_height);
    return true;
}

std::optional<std::string> SourceManager::getMetadata(std::string_view key) const 
{
    if (!isLoaded()) {
        return std::nullopt;
    }
    
    const auto& spec = m_imageBuf->spec();
    auto* attr = spec.find_attribute(std::string(key));

   // to avoid problematic implicit conversion of the ternary operator.
    if (attr) {
        return attr->get_string();
    } else {
        // Retourne std::nullopt si l'attribut n'est pas trouvé
        return std::nullopt;
    }
}

} // namespace CaptureMoment
