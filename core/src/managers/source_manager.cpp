/**
 * @file source_manager.cpp
 * @brief Implementation of SourceManager using OpenImageIO (OIIO).
 * @details Implements thread-safe image loading, RGBA conversion, and tile access.
 * @author CaptureMoment Team
 * @date 2025
 */

#include "managers/source_manager.h"
#include <algorithm>
#include <mutex>
#include <OpenImageIO/imagebufalgo.h>
#include <OpenImageIO/imageio.h>
#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::Managers {

static std::shared_ptr<OIIO::ImageCache> s_globalCachePtr = nullptr;
static std::once_flag s_cacheInitFlag;


OIIO::ImageCache* SourceManager::getGlobalCache()
{
    std::call_once(s_cacheInitFlag, [](){
        // Create the global ImageCache singleton
        s_globalCachePtr = OIIO::ImageCache::create();

        // Configure the cache
        OIIO::ImageCache* cache = s_globalCachePtr.get();
        if (cache) {
            // Limit memory usage to 2GB
            cache->attribute("max_memory_MB", 2048.0f);
            spdlog::debug("OIIO ImageCache created: 2GB limit");
        } else {
            spdlog::critical("Failed to create OIIO ImageCache.");
        }
    });
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
    unloadInternal();
    spdlog::debug("SourceManager: Instance destroyed.");
}


std::expected<bool, ErrorHandling::CoreError> SourceManager::loadFile(std::string_view path)
{
    if (path.empty()) {
        spdlog::warn("SourceManager::loadFile: Empty file path provided.");
        return false;
    }

    // Lock the mutex to ensure no other thread reads m_image_buf while we are destroying/recreating it
    std::lock_guard<std::mutex> lock(m_mutex);

    if (isLoaded_unsafe()) {
        unload(); // Cleanup old state
    }

    spdlog::info("SourceManager::loadFile: Attempting to load file: '{}'", path);

    try {
        // 1. Load the image file using OIIO
        // This loads the image in its native format (RGB, Grayscale, etc.)
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
            return std::unexpected(ErrorHandling::CoreError::DecodingError);
        }

        // ============================================================
        // OPTIMIZATION: Pre-convert Internal Buffer to RGBA_F32
        // ============================================================
        // The processing pipeline expects 4-channel RGBA Float32 data.
        // Converting here once is much faster than converting on every getTile() call.

        const int w = m_image_buf->spec().width;
        const int h = m_image_buf->spec().height;
        const int current_ch = m_image_buf->spec().nchannels;

        if (current_ch != 4)
        {
            spdlog::info("SourceManager: Converting image from {} channels to RGBA (4 channels).", current_ch);

            OIIO::ImageBuf converted;

            // Construct ImageSpec explicitly: Set format to FLOAT and channels to 4.
            OIIO::ImageSpec target_spec(w, h, 4, OIIO::TypeDesc::FLOAT);
            converted.reset(target_spec);

            // Channel mapping logic
            std::vector<int> channel_map = {0, 1, 2, 3}; // Default R, G, B, Alpha
            std::vector<float> channel_values = {0.0f, 0.0f, 0.0f, 1.0f}; // Default alpha to 1.0

            // If source is 1 channel (Grayscale), map to all RGB, Alpha to 1.0
            if (current_ch == 1) {
                channel_map = {0, 0, 0, 0};
            }

            // Use ImageBufAlgo to restructure channels.
            if (!OIIO::ImageBufAlgo::channels(*m_image_buf, converted, 4, channel_map, channel_values)) {
                spdlog::error("SourceManager: Failed to convert channels to RGBA.");
                m_image_buf.reset();
                return std::unexpected(ErrorHandling::CoreError::AllocationFailed);
            }

            // Replace the original buffer with the optimized RGBA version
            *m_image_buf = converted;
        }
        else {
            // Already 4 channels. Ensure format is FLOAT if it wasn't (e.g. UINT8).
            if (m_image_buf->spec().format != OIIO::TypeDesc::FLOAT)
            {
                spdlog::info("SourceManager: Converting pixel format to FLOAT.");

                // Create a new buffer with the correct format (FLOAT)
                OIIO::ImageSpec target_spec(w, h, 4, OIIO::TypeDesc::FLOAT);
                OIIO::ImageBuf converted(target_spec);

                // Copy pixels from current buffer into the new buffer, converting format
                if (!OIIO::ImageBufAlgo::copy(converted, *m_image_buf, OIIO::TypeDesc::FLOAT))
                {
                    spdlog::error("SourceManager: Failed to convert pixel format to FLOAT.");
                    m_image_buf.reset();
                    return std::unexpected(ErrorHandling::CoreError::AllocationFailed);
                }

                *m_image_buf = converted;
            }
        }

        m_current_path = path;

        spdlog::info("SourceManager: Successfully loaded '{}'. Internal Resolution: {}x{} (4 channels RGBA_F32).",
                     m_current_path, width(), height());
        return true;

    } catch (const std::bad_alloc&) {
        // Capture spécifique du manque de mémoire
        spdlog::critical("SourceManager::loadFile: Memory allocation failed for '{}'.", path);
        m_image_buf.reset();
        return std::unexpected(ErrorHandling::CoreError::AllocationFailed);
    }
    catch (const std::exception& e) {
        spdlog::critical("SourceManager::loadFile: C++ Exception during file loading of '{}': {}",
                         path, e.what());
        m_image_buf.reset();
        return std::unexpected(ErrorHandling::CoreError::Unexpected);
    }
}

void SourceManager::unload()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    unloadInternal();
}

void SourceManager::unloadInternal()
{
    if (m_image_buf && m_image_buf->initialized()) {
        spdlog::info("SourceManager: Unloading '{}'.", m_current_path);
    }
    m_image_buf.reset();
    m_current_path.clear();
}

bool SourceManager::isLoaded() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return isLoaded_unsafe();
}

bool SourceManager::isLoaded_unsafe() const {
    return m_image_buf && m_image_buf->initialized();
}

int SourceManager::width() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return (m_image_buf && m_image_buf->initialized()) ? m_image_buf->spec().width : 0;
}

int SourceManager::height() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return (m_image_buf && m_image_buf->initialized()) ? m_image_buf->spec().height : 0;
}

std::unique_ptr<Common::ImageRegion> SourceManager::getTile(
    int x, int y, int width, int height)
{
    // 1. Lock the mutex to protect m_image_buf from being deleted/modified by loadFile/unload
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_image_buf || !m_image_buf->initialized()) {
        spdlog::warn("SourceManager::getTile: Called but no image loaded");
        return nullptr;
    }

    // Retrieve dimensions directly from spec to avoid re-locking via width()/height()
    const int img_w = m_image_buf->spec().width;
    const int img_h = m_image_buf->spec().height;

    if (width <= 0 || height <= 0) {
        spdlog::warn("SourceManager::getTile: Invalid dimensions requested ({}x{})", width, height);
        return nullptr;
    }

    // 2. Clamp coordinates to image bounds to prevent out-of-range access
    int clamped_x = std::clamp(x, 0, img_w - 1);
    int clamped_y = std::clamp(y, 0, img_h - 1);
    int clamped_width = std::min(width, img_w - clamped_x);
    int clamped_height = std::min(height, img_h - clamped_y);

    if (clamped_width <= 0 || clamped_height <= 0) {
        spdlog::warn("SourceManager::getTile: Clamped region has zero area ({}x{})", clamped_width, clamped_height);
        return nullptr;
    }

    spdlog::trace("SourceManager::getTile: Clamped region: ({}, {}) size: {}x{}",
                  clamped_x, clamped_y, clamped_width, clamped_height);

    // 3. Prepare ImageRegion structure
    auto region = std::make_unique<Common::ImageRegion>();
    region->m_x = clamped_x;
    region->m_y = clamped_y;
    region->m_width = clamped_width;
    region->m_height = clamped_height;
    region->m_channels = 4;
    region->m_format = Common::PixelFormat::RGBA_F32;

    // 4. Allocate memory for pixel data
    const size_t dataSize = static_cast<size_t>(clamped_width * clamped_height * 4);
    region->m_data.resize(dataSize);

    // 5. Define OIIO ROI (Region of Interest) for channels 0-3
    OIIO::ROI roi(
        clamped_x, clamped_x + clamped_width,
        clamped_y, clamped_y + clamped_height,
        0, 1,  // Z (depth)
        0, 4   // Channels (RGBA)
        );

    // 6. Direct Copy from buffer to region data
    if (!m_image_buf->get_pixels(roi, OIIO::TypeDesc::FLOAT, region->m_data.data())) {
        spdlog::warn("SourceManager::getTile: Failed to extract tile at ({}, {})", clamped_x, clamped_y);
        return nullptr;
    }

    spdlog::trace("SourceManager::getTile: Tile extracted successfully.");
    return region;
}

bool SourceManager::setTile(const Common::ImageRegion& tile)
{
    // 1. Lock the mutex to prevent read/write race conditions
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_image_buf || !m_image_buf->initialized()) {
        spdlog::warn("SourceManager::setTile: Called but no image loaded");
        return false;
    }

    if (!tile.isValid()) {
        spdlog::error("SourceManager::setTile: Received an invalid ImageRegion.");
        return false;
    }

    // Check format compatibility
    if (tile.m_format != Common::PixelFormat::RGBA_F32) {
        spdlog::error("SourceManager::setTile: ImageRegion format is not RGBA_F32 (got {}).", static_cast<int>(tile.m_format));
        return false;
    }

    if (tile.m_channels != 4) {
        spdlog::error("SourceManager::setTile: ImageRegion does not have 4 channels (got {}).", tile.m_channels);
        return false;
    }

    // Retrieve dimensions directly
    const int img_w = m_image_buf->spec().width;
    const int img_h = m_image_buf->spec().height;

    // Check bounds
    if (tile.m_x < 0 || tile.m_y < 0 ||
        tile.m_x + tile.m_width > img_w ||
        tile.m_y + tile.m_height > img_h) {
        spdlog::error("SourceManager::setTile: Tile coordinates ({},{}) size ({}x{}) are out of image bounds ({}x{}).",
                      tile.m_x, tile.m_y, tile.m_width, tile.m_height, img_w, img_h);
        return false;
    }

    // 2. Define OIIO ROI
    OIIO::ROI roi(
        tile.m_x, tile.m_x + tile.m_width,
        tile.m_y, tile.m_y + tile.m_height,
        0, 1, // Z
        0, 4  // Channels
        );

    // 3. Write pixels to the buffer
    if (!m_image_buf->set_pixels(roi, OIIO::TypeDesc::FLOAT, tile.m_data.data())) {
        spdlog::error("SourceManager::setTile: Failed to write tile back at ({}, {}). OIIO Error: {}",
                      tile.m_x, tile.m_y, m_image_buf->geterror());
        return false;
    }

    spdlog::trace("SourceManager::setTile: Tile written back successfully.");
    return true;
}

std::optional<std::string> SourceManager::getMetadata(std::string_view key) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_image_buf) {
        return std::nullopt;
    }

    const auto& spec = m_image_buf->spec();
    // find_attribute takes a std::string, so we must convert view
    auto* attr = spec.find_attribute(std::string(key));

    if (attr) {
        return attr->get_string();
    } else {
        return std::nullopt;
    }
}

} // namespace CaptureMoment::Core::Managers
