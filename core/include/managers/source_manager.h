/**
 * @file source_manager.h
 * @brief Image source management class using OpenImageIO (OIIO).
 *
 * @details
 * Concrete implementation of ISourceManager.
 *
 * Features:
 * - Thread-safe access via `std::mutex`.
 * - Pre-conversion of source images to RGBA_F32 during `loadFile` to optimize
 *   subsequent tile access.
 * - Relies on a global OIIO ImageCache singleton for resource management.
 *
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include "managers/i_source_manager.h"

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagecache.h>

#include <mutex>
#include <string>
#include <memory>

namespace CaptureMoment::Core {

namespace Managers {

/**
 * @class SourceManager
 * @brief Concrete implementation of ISourceManager using OIIO.
 *
 * Manages the lifetime of an `OIIO::ImageBuf`. Ensures that all internal
 * representations are converted to the standard RGBA_F32 format upon loading.
 */
class SourceManager : public ISourceManager {
public:
    SourceManager();
    ~SourceManager() override;

    // -----------------------------------------------------------------
    // ISourceManager Implementation
    // -----------------------------------------------------------------

    [[nodiscard]] std::expected<void, ErrorHandling::CoreError> loadFile(std::string_view path) override;
    void unload() override;
    [[nodiscard]] bool isLoaded() const override;
    [[nodiscard]] int width() const noexcept override;
    [[nodiscard]] int height() const noexcept override;
    [[nodiscard]] int channels() const noexcept override;

    [[nodiscard]] std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError> getTile(
        int x, int y, int width, int height
        ) override;

    [[nodiscard]] std::expected<void, ErrorHandling::CoreError> setTile(const Common::ImageRegion& tile) override;

    [[nodiscard]] std::optional<std::string> getMetadata(std::string_view key) const override;

private:
    /**
     * @brief The main image buffer containing the loaded image data.
     * Managed by std::unique_ptr (RAII).
     */
    std::unique_ptr<OIIO::ImageBuf> m_image_buf;

    /**
     * @brief Mutex protecting access to m_image_buf, m_current_path, and state changes.
     * Mutable to allow locking in const methods.
     */
    mutable std::mutex m_mutex;

    /**
     * @brief Pointer to OIIO's global image cache.
     * This class does NOT manage the lifetime of the ImageCache object.
     */
    OIIO::ImageCache* m_cache;

    /**
     * @brief The file path of the currently loaded image.
     */
    std::string m_current_path;

    /**
     * @brief Provides access to OIIO's global ImageCache singleton.
     */
    static OIIO::ImageCache* getGlobalCache();

    /**
     * @brief Internal helper to unload the image without locking.
     * Assumes caller holds m_mutex.
     */
    void unloadInternal();

    /**
     * @brief Internal helper to check load status without locking.
     * Assumes caller holds m_mutex.
     */
    [[nodiscard]] bool isLoaded_unsafe() const;
};

} // namespace Managers

} // namespace CaptureMoment::Core
