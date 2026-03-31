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
    [[nodiscard]] Common::ImageDim width() const noexcept override;
    [[nodiscard]] Common::ImageDim height() const noexcept override;
    [[nodiscard]] Common::ImageChan channels() const noexcept override;

    [[nodiscard]] std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError> getTile(
        Common::ImageDim x, Common::ImageDim y, Common::ImageDim width, Common::ImageDim height
        ) override;

    [[nodiscard]] std::expected<void, ErrorHandling::CoreError> setTile(const Common::ImageRegion& tile) override;

    [[nodiscard]] std::optional<std::string> getMetadata(std::string_view key) const override;

    [[nodiscard]] std::string getImageSourcePath() const override;

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


    /**
     * @brief Loads an image file via OIIO ImageCache into an ImageBuf (F32).
     * @param path The file path to load.
     * @param config Optional OIIO configuration attributes (for RAW, HEIF, etc.).
     * @return The loaded ImageBuf on success, or a CoreError on failure.
     */
    [[nodiscard]] std::expected<OIIO::ImageBuf, ErrorHandling::CoreError>
    loadImageBuffer(std::string_view path, const OIIO::ImageSpec* config = nullptr);

    // ============================================================
    // RAW File Handling
    // ============================================================

    /**
     * @brief Checks if a file path refers to a RAW image format.
     *
     * Determines whether the file extension corresponds to a RAW camera format
     * (e.g., .NEF, .CR2, .ARW, .DNG) that requires special processing.
     *
     * @param path The file path to check.
     * @return true If the file is a RAW format.
     * @return false If the file is a standard image format (JPEG, PNG, TIFF).
     */
    [[nodiscard]] bool isRawFile(std::string_view path) const;

    /**
     * @brief Checks if a file path refers to a HEIC image format.
     *
     * Determines whether the file extension corresponds to a HEIC format
     * that may require special processing (e.g., reorientation).
     *
     * @param path The file path to check.
     * @return true If the file is a HEIC format.
     * @return false If the file is not a HEIC format.
     */
    [[nodiscard]] bool isHeicFile(std::string_view path) const;

    /**
     * @brief Loads a RAW image file with optimized LibRaw settings.
     *
     * This method handles RAW camera files (NEF, CR2, ARW, DNG, etc.) by configuring
     * OIIO/LibRaw with appropriate settings for photo editing:
     * - Full resolution output
     * - AHD demosaicing algorithm
     * - Linear color space (sRGB-linear)
     * - Camera white balance preserved
     * - Highlight recovery enabled
     *
     * @param path The path to the RAW file.
     * @return std::expected<void, CoreError> Success or error code.
     */
    [[nodiscard]] std::expected<void, ErrorHandling::CoreError> loadRawFile(std::string_view path);

    /**
     * @brief Loads a HEIC image file with optimized libheif settings.
     *
     * This method handles HEIC files, which often require special handling for
     * orientation and alpha. It configures libheif to:
     * - Automatically reorient based on EXIF metadata (recommended for smartphone photos).
     * - Preserve unassociated alpha if present.
     *
     * @param path The path to the HEIC file.
     * @return std::expected<void, CoreError> Success or error code.
     */
    [[nodiscard]] std::expected<void, ErrorHandling::CoreError> loadHeicFile(std::string_view path);

    /**
     * @brief Loads a standard image file (JPEG, PNG, TIFF, etc.).
     *
     * This method handles non-RAW image formats using OIIO ImageCache.
     * No special RAW processing is applied.
     *
     * @param path The path to the image file.
     * @return std::expected<void, CoreError> Success or error code.
     */
    [[nodiscard]] std::expected<void, ErrorHandling::CoreError> loadStandardFile(std::string_view path);

    /**
     * @brief Converts an OIIO ImageBuf to RGBA_F32 internal format.
     *
     * @param src_buf Source buffer (any format).
     * @return std::expected<void, CoreError> Success or error code.
     */
    [[nodiscard]] std::expected<void, ErrorHandling::CoreError> convertToRgbaInternal(OIIO::ImageBuf&& src_buf);
};

} // namespace Managers

} // namespace CaptureMoment::Core
