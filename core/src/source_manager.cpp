/**
 * @file source_manager.cpp
 * @brief Image source management class using OpenImageIO (OIIO).
 * @author CaptureMoment Team
 * @date 2025
 */

#include "spdlog/spdlog.h"

#include "source_manager.h"
#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagecache.h>
#include <algorithm>

namespace CaptureMoment {

static OIIO::ImageCache* s_globalCache = nullptr;

OIIO::ImageCache* SourceManager::getGlobalCache() {
    if (!s_globalCache) {
        s_globalCache = OIIO::ImageCache::create();
        s_globalCache->attribute("max_memory_MB", 2048.0f);

        spdlog::debug("OIIO ImageCache created: 2GB");
    }
    return s_globalCache;
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
            m_cache
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

int SourceManager::width() const {
    return isLoaded() ? m_imageBuf->spec().width : 0;
}

int SourceManager::height() const {
    return isLoaded() ? m_imageBuf->spec().height : 0;
}

int SourceManager::channels() const {
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

std::optional<std::string> SourceManager::getMetadata(std::string_view key) const {
    if (!isLoaded()) {
        return std::nullopt;
    }
    
    const auto& spec = m_imageBuf->spec();
    auto* attr = spec.find_attribute(std::string(key));

    return attr ? attr->get_string() : std::nullopt;
}

} // namespace CaptureMoment