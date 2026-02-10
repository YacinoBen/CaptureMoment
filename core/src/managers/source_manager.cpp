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

// Global static members for cache management
static std::shared_ptr<OIIO::ImageCache> s_globalCachePtr = nullptr;
static std::once_flag s_cacheInitFlag;

OIIO::ImageCache* SourceManager::getGlobalCache()
{
    std::call_once(s_cacheInitFlag, [](){
        // Create the global ImageCache singleton
        s_globalCachePtr = OIIO::ImageCache::create();

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
        spdlog::critical("SourceManager: Failed to get global ImageCache.");
        throw std::runtime_error("SourceManager: Initialization failed.");
    }
}

SourceManager::~SourceManager()
{
    unloadInternal();
}

std::expected<void, ErrorHandling::CoreError> SourceManager::loadFile(std::string_view path)
{
    if (path.empty()) {
        spdlog::warn("SourceManager::loadFile: Empty file path provided.");
        return std::unexpected(ErrorHandling::CoreError::FileNotFound);
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    if (isLoaded_unsafe()) {
        unload();
    }

    spdlog::info("SourceManager: Loading file: '{}'", path);

    try {
        // 1. Load the image using OIIO
        m_image_buf = std::make_unique<OIIO::ImageBuf>(
            std::string(path),
            0, 0,
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
        // OPTIMIZATION: Pre-convert to RGBA_F32
        // ============================================================
        const int w = m_image_buf->spec().width;
        const int h = m_image_buf->spec().height;
        const int current_ch = m_image_buf->spec().nchannels;

        if (current_ch != 4)
        {
            spdlog::info("SourceManager: Converting image from {} channels to RGBA (4 channels).", current_ch);

            OIIO::ImageBuf converted;
            OIIO::ImageSpec target_spec(w, h, 4, OIIO::TypeDesc::FLOAT);
            converted.reset(target_spec);

            std::vector<int> channel_map = {0, 1, 2, 3};
            std::vector<float> channel_values = {0.0f, 0.0f, 0.0f, 1.0f};

            if (current_ch == 1) {
                channel_map = {0, 0, 0, 0}; // Grayscale to RGB
            }

            if (!OIIO::ImageBufAlgo::channels(*m_image_buf, converted, 4, channel_map, channel_values)) {
                spdlog::error("SourceManager: Failed to convert channels to RGBA.");
                m_image_buf.reset();
                return std::unexpected(ErrorHandling::CoreError::AllocationFailed);
            }

            *m_image_buf = converted;
        }
        else {
            // Ensure format is FLOAT if it wasn't (e.g. UINT8)
            if (m_image_buf->spec().format != OIIO::TypeDesc::FLOAT)
            {
                spdlog::info("SourceManager: Converting pixel format to FLOAT.");

                OIIO::ImageSpec target_spec(w, h, 4, OIIO::TypeDesc::FLOAT);
                OIIO::ImageBuf converted(target_spec);

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
        return {};

    } catch (const std::bad_alloc&) {
        spdlog::critical("SourceManager::loadFile: Memory allocation failed for '{}'.", path);
        m_image_buf.reset();
        return std::unexpected(ErrorHandling::CoreError::AllocationFailed);
    } catch (const std::exception& e) {
        spdlog::critical("SourceManager::loadFile: Exception during file loading of '{}': {}", path, e.what());
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

int SourceManager::channels() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return (m_image_buf && m_image_buf->initialized()) ? m_image_buf->spec().nchannels : 0;
}

std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError> SourceManager::getTile(
    int x, int y, int width, int height)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_image_buf || !m_image_buf->initialized()) {
        spdlog::warn("SourceManager::getTile: No image loaded");
        return std::unexpected(ErrorHandling::CoreError::SourceNotLoaded);
    }

    const int img_w = m_image_buf->spec().width;
    const int img_h = m_image_buf->spec().height;

    if (width <= 0 || height <= 0) {
        return std::unexpected(ErrorHandling::CoreError::InvalidImageRegion);
    }

    // Clamp coordinates to image bounds
    int clamped_x = std::clamp(x, 0, img_w - 1);
    int clamped_y = std::clamp(y, 0, img_h - 1);
    int clamped_width = std::min(width, img_w - clamped_x);
    int clamped_height = std::min(height, img_h - clamped_y);

    if (clamped_width <= 0 || clamped_height <= 0) {
        return std::unexpected(ErrorHandling::CoreError::InvalidImageRegion);
    }

    auto region = std::make_unique<Common::ImageRegion>();
    region->m_x = clamped_x;
    region->m_y = clamped_y;
    region->m_width = clamped_width;
    region->m_height = clamped_height;
    region->m_channels = 4;
    region->m_format = Common::PixelFormat::RGBA_F32;

    const size_t dataSize = static_cast<size_t>(clamped_width * clamped_height * 4);
    region->m_data.resize(dataSize);

    OIIO::ROI roi(
        clamped_x, clamped_x + clamped_width,
        clamped_y, clamped_y + clamped_height,
        0, 1,
        0, 4
        );

    if (!m_image_buf->get_pixels(roi, OIIO::TypeDesc::FLOAT, region->m_data.data())) {
        spdlog::warn("SourceManager::getTile: Failed to extract tile");
        return std::unexpected(ErrorHandling::CoreError::IOError);
    }

    return region;
}

std::expected<void, ErrorHandling::CoreError> SourceManager::setTile(const Common::ImageRegion& tile)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_image_buf || !m_image_buf->initialized()) {
        spdlog::warn("SourceManager::setTile: No image loaded");
        return std::unexpected(ErrorHandling::CoreError::SourceNotLoaded);
    }

    if (!tile.isValid()) {
        return std::unexpected(ErrorHandling::CoreError::InvalidImageRegion);
    }

    if (tile.m_format != Common::PixelFormat::RGBA_F32 || tile.m_channels != 4) {
        return std::unexpected(ErrorHandling::CoreError::InvalidImageRegion);
    }

    const int img_w = m_image_buf->spec().width;
    const int img_h = m_image_buf->spec().height;

    if (tile.m_x < 0 || tile.m_y < 0 ||
        tile.m_x + tile.m_width > img_w ||
        tile.m_y + tile.m_height > img_h) {
        return std::unexpected(ErrorHandling::CoreError::InvalidImageRegion);
    }

    OIIO::ROI roi(
        tile.m_x, tile.m_x + tile.m_width,
        tile.m_y, tile.m_y + tile.m_height,
        0, 1,
        0, 4
        );

    if (!m_image_buf->set_pixels(roi, OIIO::TypeDesc::FLOAT, tile.m_data.data())) {
        spdlog::error("SourceManager::setTile: Failed to write tile. OIIO Error: {}", m_image_buf->geterror());
        return std::unexpected(ErrorHandling::CoreError::IOError);
    }

    return {};
}

std::optional<std::string> SourceManager::getMetadata(std::string_view key) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_image_buf) {
        return std::nullopt;
    }

    const auto& spec = m_image_buf->spec();
    auto* attr = spec.find_attribute(std::string(key));

    if (attr) {
        return attr->get_string();
    } else {
        return std::nullopt;
    }
}

} // namespace CaptureMoment::Core::Managers
