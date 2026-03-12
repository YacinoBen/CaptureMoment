/**
 * @file source_manager.cpp
 * @brief Implementation of SourceManager
 * @author CaptureMoment Team
 * @date 2025
 */

#include "managers/source_manager.h"

#include <OpenImageIO/imagebufalgo.h>
#include <OpenImageIO/imageio.h>
#include <spdlog/spdlog.h>
#include <mutex>
#include <algorithm>

namespace CaptureMoment::Core::Managers {

// Global static members for cache management
static std::shared_ptr<OIIO::ImageCache> s_globalCachePtr = nullptr;
static std::once_flag s_cacheInitFlag;

OIIO::ImageCache* SourceManager::getGlobalCache()
{
    std::call_once(s_cacheInitFlag, []() {
        s_globalCachePtr = OIIO::ImageCache::create();
        
        if (s_globalCachePtr) {
            s_globalCachePtr->attribute("max_memory_MB", 2048.0f);
            spdlog::debug("[SourceManager] OIIO ImageCache created: 2GB limit");
        } else {
            spdlog::critical("[SourceManager] Failed to create OIIO ImageCache");
        }
    });
    return s_globalCachePtr.get();
}

SourceManager::SourceManager()
    : m_cache(getGlobalCache())
{
    if (!m_cache) {
        spdlog::critical("[SourceManager] Failed to get global ImageCache");
        throw std::runtime_error("SourceManager: Initialization failed");
    }
}

SourceManager::~SourceManager()
{
    unloadInternal();
}

std::expected<void, ErrorHandling::CoreError> SourceManager::loadFile(std::string_view path)
{
    if (path.empty()) {
        spdlog::warn("[SourceManager::loadFile] Empty file path");
        return std::unexpected(ErrorHandling::CoreError::FileNotFound);
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    if (isLoaded_unsafe()) {
        unloadInternal();
    }

    spdlog::info("[SourceManager::loadFile] Loading: '{}'", path);

    try {
        // ============================================================
        // Step 1: Load via OIIO ImageCache
        // ============================================================
        OIIO::ImageBuf src_buf(std::string(path), 0, 0, s_globalCachePtr);
        
        if (!src_buf.read(0, 0, true, OIIO::TypeDesc::FLOAT)) {
            spdlog::error("[SourceManager::loadFile] Failed to read '{}': {}", 
                          path, src_buf.geterror());
            return std::unexpected(ErrorHandling::CoreError::DecodingError);
        }

        // ============================================================
        // Step 2: Get dimensions
        // ============================================================
        const Common::ImageDim w = static_cast<Common::ImageDim>(src_buf.spec().width);
        const Common::ImageDim h = static_cast<Common::ImageDim>(src_buf.spec().height);
        const Common::ImageChan src_ch = static_cast<Common::ImageChan>(src_buf.spec().nchannels);

        // ============================================================
        // Step 3: Convert to RGBA_F32 (standard internal format)
        // ============================================================
        OIIO::ImageSpec target_spec(static_cast<int>(w), static_cast<int>(h), 4, OIIO::TypeDesc::FLOAT);
        OIIO::ImageBuf rgba_buf(target_spec);

        // Channel mapping for conversion
        std::vector<int> channel_order = {0, 1, 2, 3};
        std::vector<float> channel_values = {0.0f, 0.0f, 0.0f, 1.0f};

        switch (src_ch) {
            case 1:  // Grayscale → R=G=B=gray, A=1
                channel_order = {0, 0, 0, -1};
                break;
            case 2:  // Gray+Alpha → RGB=gray, A=alpha
                channel_order = {0, 0, 0, 1};
                break;
            case 3:  // RGB → RGB, A=1
                channel_order = {0, 1, 2, -1};
                break;
            case 4:  // RGBA → direct copy
            default:
                channel_order = {0, 1, 2, 3};
                break;
        }

        if (!OIIO::ImageBufAlgo::channels(rgba_buf, src_buf, 4, channel_order, channel_values)) {
            spdlog::error("[SourceManager::loadFile] Channel conversion failed: {}", rgba_buf.geterror());
            return std::unexpected(ErrorHandling::CoreError::AllocationFailed);
        }

        // ============================================================
        // Step 4: Store final buffer
        // ============================================================
        m_image_buf = std::make_unique<OIIO::ImageBuf>(std::move(rgba_buf));
        m_current_path = path;

        spdlog::info("[SourceManager::loadFile] Loaded '{}': {}x{} RGBA_F32", 
                     m_current_path, w, h);
        return {};

    } catch (const std::bad_alloc&) {
        spdlog::critical("[SourceManager::loadFile] Allocation failed for '{}'", path);
        m_image_buf.reset();
        return std::unexpected(ErrorHandling::CoreError::AllocationFailed);
    } catch (const std::exception& e) {
        spdlog::critical("[SourceManager::loadFile] Exception: {}", e.what());
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
        spdlog::info("[SourceManager] Unloading '{}'", m_current_path);
    }
    m_image_buf.reset();
    m_current_path.clear();
}

bool SourceManager::isLoaded() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return isLoaded_unsafe();
}

bool SourceManager::isLoaded_unsafe() const
{
    return m_image_buf && m_image_buf->initialized();
}

Common::ImageDim SourceManager::width() const noexcept
{
    return m_image_buf ? static_cast<Common::ImageDim>(m_image_buf->spec().width) : 0;
}

Common::ImageDim SourceManager::height() const noexcept
{
    return m_image_buf ? static_cast<Common::ImageDim>(m_image_buf->spec().height) : 0;
}

Common::ImageChan SourceManager::channels() const noexcept
{
    return m_image_buf ? static_cast<Common::ImageChan>(m_image_buf->spec().nchannels) : 0;
}

std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError>
SourceManager::getTile(Common::ImageDim x, Common::ImageDim y, 
                       Common::ImageDim width, Common::ImageDim height)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!isLoaded_unsafe()) {
        spdlog::warn("[SourceManager::getTile] No image loaded");
        return std::unexpected(ErrorHandling::CoreError::SourceNotLoaded);
    }

    if (width == 0 || height == 0) {
        return std::unexpected(ErrorHandling::CoreError::InvalidImageRegion);
    }

    // OIIO handles bounds via ROI - no manual clamping needed
    OIIO::ROI roi(
        static_cast<int>(x), static_cast<int>(x + width),
        static_cast<int>(y), static_cast<int>(y + height),
        0, 1,
        0, 4
    );

    // Allocate result
    const size_t dataSize = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
    std::vector<float> data(dataSize);

    if (!m_image_buf->get_pixels(roi, OIIO::TypeDesc::FLOAT, data.data())) {
        spdlog::warn("[SourceManager::getTile] get_pixels failed: {}", m_image_buf->geterror());
        return std::unexpected(ErrorHandling::CoreError::IOError);
    }

    // Create ImageRegion
    auto region = std::make_unique<Common::ImageRegion>(
        std::move(data),
        width,
        height,
        static_cast<Common::ImageChan>(4)
    );
    region->m_x = static_cast<Common::ImageCoord>(x);
    region->m_y = static_cast<Common::ImageCoord>(y);
    region->m_format = Common::PixelFormat::RGBA_F32;

    return region;
}

std::expected<void, ErrorHandling::CoreError>
SourceManager::setTile(const Common::ImageRegion& tile)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!isLoaded_unsafe()) {
        spdlog::warn("[SourceManager::setTile] No image loaded");
        return std::unexpected(ErrorHandling::CoreError::SourceNotLoaded);
    }

    if (!tile.isValid()) {
        return std::unexpected(ErrorHandling::CoreError::InvalidImageRegion);
    }

    if (tile.m_format != Common::PixelFormat::RGBA_F32 || tile.m_channels != 4) {
        spdlog::error("[SourceManager::setTile] Invalid format: expected RGBA_F32");
        return std::unexpected(ErrorHandling::CoreError::InvalidImageRegion);
    }

    // OIIO ROI for the tile
    OIIO::ROI roi(
        static_cast<int>(tile.m_x), static_cast<int>(tile.m_x + tile.m_width),
        static_cast<int>(tile.m_y), static_cast<int>(tile.m_y + tile.m_height),
        0, 1,
        0, 4
    );

    if (!m_image_buf->set_pixels(roi, OIIO::TypeDesc::FLOAT, tile.m_data.data())) {
        spdlog::error("[SourceManager::setTile] set_pixels failed: {}", m_image_buf->geterror());
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
    const auto* attr = spec.find_attribute(std::string(key));

    return attr ? std::optional{attr->get_string()} : std::nullopt;
}

std::string SourceManager::getImageSourcePath() const
{
    std::lock_guard lock(m_mutex);
    return m_current_path;
}

} // namespace CaptureMoment::Core::Managers
