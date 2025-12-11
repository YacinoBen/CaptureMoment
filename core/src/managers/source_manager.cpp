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

OIIO::ImageCache* SourceManager::getGlobalCache() {
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

SourceManager::~SourceManager() {
    unload();
    spdlog::debug("SourceManager: Instance destroyed.");
}

bool SourceManager::loadFile(std::string_view path) {

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
        
        if (!m_imageBuf->read()) {
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

void SourceManager::unload() {

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
    int x, int y, int width, int height
) {
    if (!isLoaded()) {
        spdlog::warn("getTile() called but no image loaded");
        return nullptr;
    }
    
    // 1. Clamping coordinates to stay within bounds
    // Note: OIIO ROI uses exclusive max coordinates, but we clamp the requested size here.

    int clamped_x = std::clamp(x, 0, this->width());
    int clamped_y = std::clamp(y, 0, this->height());
    int clamped_width = std::min(width, this->width() - clamped_x);
    int clamped_height = std::min(height, this->height() - clamped_y);
    
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
    
    // 3. Definition of the OIIO Region of Interest (ROI)
    OIIO::ROI roi(clamped_x, clamped_x + clamped_width, clamped_y, clamped_y + clamped_height, 
        0, 1, // Zmin, Zmax
        0, 4); // Cmin, Cmax (force 4 channels in the OIIO call)
    
    // 4. Extraction of pixels
    // get_pixels() reads the pixels into the destination memory buffer (region->m_data.data())
    // using the specified type (OIIO::TypeDesc::FLOAT) and ROI.

    if (!m_imageBuf->get_pixels(roi, OIIO::TypeDesc::FLOAT, region->m_data.data())) {
        spdlog::warn("Failed to extract tile at ({}, {}) size {}x{}", clamped_x, clamped_y, clamped_width, clamped_height);
        return nullptr;
    }
        
    spdlog::trace("SourceManager: Tile extracted: ({}, {}) {}x{} to Float/RGBA.", 
                  clamped_x, clamped_y, clamped_width, clamped_height);
    return region;
}

bool SourceManager::setTile(const ImageRegion& tile) {
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

std::optional<std::string> SourceManager::getMetadata(std::string_view key) const {
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
