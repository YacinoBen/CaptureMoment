/**
 * @file display_manager.h
 * @brief Manages image display with downsampling (with OIIO), zoom, and pan.
 * @author CaptureMoment Team
 * @date 2025
 * * Architecture:
 * - Backend (SourceManager): Full resolution image (untouched, e.g., 4928x3264)
 * - DisplayManager: Downsampled image for display (e.g., 1920x1280)
 * - Frontend (IRenderingItemBase): Display-ready image (PaintedImageItem/RHIImageItem/SGSImageItem)
 * * This separation ensures:
 * 1. Operations work on full resolution
 * 2. Display is optimized for screen size
 * 3. Memory usage is minimized
 */

#pragma once

#include <memory>
#include <QObject>
#include <QSize>
#include <QPointF>

#include "common/image_region.h"
#include "viewport_manager.h"

// Forward declaration
namespace CaptureMoment::UI::Rendering {
class IRenderingItemBase;
}

namespace CaptureMoment::UI {

/**
 * @brief Namespace containing Qt-specific UI components for CaptureMoment.
 * * This namespace includes classes responsible for rendering and UI integration
 * using Qt Quick and the Qt Rendering Hardware Interface (QRhi).
 */
namespace Display {

/**
 * @class DisplayManager
 * @brief Central manager for image display, zoom, pan, and viewport coordination.
 *
 * This class serves as the single point of coordination for:
 * - Screen and viewport information (via ViewportManager)
 * - Source and display image size tracking
 * - Zoom and pan state management
 * - Rendering item coordination
 *
 * Designed for QML integration:
 * - All properties are Q_PROPERTY for QML binding
 * - All public methods are Q_INVOKABLE for QML calls
 * - Signals for reactive UI updates
 *
 * ## Usage Flow
 *
 * 1. **Initialization (once at app startup)**:
 *    - Call `initialize()` with screen info and initial viewport
 *    - ViewportManager calculates fixed MAX_DOWNSAMPLE
 *
 * 2. **Image Loading (per image)**:
 *    - SourceManager loads full resolution image
 *    - Call `setSourceImageSize()` with source dimensions
 *    - DisplayManager calculates optimal downsample size
 *    - Signal `displayImageRequest()` emitted
 *    - SourceManager downsamples and provides via `createDisplayImage()`
 *
 * 3. **Display**:
 *    - Rendering item receives the downsampled image
 *    - Call `fitToView()` to center and fit the image
 */
class DisplayManager : public QObject {
    Q_OBJECT

    /** @property sourceImageSize The resolution of the original backend image. */
    Q_PROPERTY(QSize sourceImageSize READ sourceImageSize NOTIFY sourceImageSizeChanged)

    /** @property displayImageSize The resolution of the downsampled buffer. */
    Q_PROPERTY(QSize displayImageSize READ displayImageSize NOTIFY displayImageSizeChanged)

    /** @property Downsampled image size (GPU texture size). */
    Q_PROPERTY(QSize downsampleSize READ downsampleSize NOTIFY downsampleSizeChanged)

    /** @property zoom The current magnification level of the displayed image. */
    Q_PROPERTY(float zoom READ zoom WRITE setZoom NOTIFY zoomChanged)

    /** @property pan The current translation offset of the displayed image. */
    Q_PROPERTY(QPointF pan READ pan WRITE setPan NOTIFY panChanged)

    /** @property viewportSize The size of the UI container area. */
    Q_PROPERTY(QSize viewportSize READ viewportSize WRITE setViewportSize NOTIFY viewportSizeChanged)

    /** @property displayScale Ratio between display and source dimensions. */
    Q_PROPERTY(float displayScale READ displayScale NOTIFY displayScaleChanged)

    /** @property Maximum downsample dimension in pixels. */
    Q_PROPERTY(int maxDownsample READ maxDownsample NOTIFY maxDownsampleChanged)


public:
    /**
     * @brief Constructs a DisplayManager.
     * @param parent Optional Qt parent object.
     */
    explicit DisplayManager(QObject* parent = nullptr);

    DisplayManager(const DisplayManager&) = delete;
    DisplayManager& operator=(const DisplayManager&) = delete;
    DisplayManager(DisplayManager&&) = default;
    DisplayManager& operator=(DisplayManager&&) = default;
    
    /** @brief Default destructor. */
    ~DisplayManager() = default;
    

    // =========================================================================
    // Initialization
    // =========================================================================

    /**
     * @brief Initializes with auto-detection of screen properties.
     *
     * Uses Qt to detect primary screen dimensions and device pixel ratio.
     * Default viewport size is 800x600.
     */
    void initialize();

    /**
     * @brief Initializes the display manager with screen and viewport parameters.
     *
     * This must be called once at application startup before loading any images.
     * It configures the ViewportManager with screen characteristics which then
     * calculates the fixed MAX_DOWNSAMPLE value.
     *
     * @param viewport_logical Initial viewport size in QML logical coordinates.
     * @param screen Physical screen information (size in pixels, DPR).
     */
    Q_INVOKABLE void initialize(const QSize& viewport_logical, const ScreenInfo& screen);

    /**
     * @brief Checks if the manager has been initialized.
     * @return True if initialize() has been called successfully.
     */
    [[nodiscard]] bool isInitialized() const noexcept;

    // =========================================================================
    // Rendering Item
    // =========================================================================

    /**
     * @brief Assigns the rendering item that will receive image updates.
     *
     * The rendering item is responsible for GPU texture management and
     * actual drawing. DisplayManager coordinates with it for:
     * - Image data transfer (setImage)
     * - Zoom/pan synchronization
     *
     * @param item Pointer to an IRenderingItemBase implementation.
     */
    void setRenderingItem(Rendering::IRenderingItemBase* item);
    
    /**
     * @brief Gets the current rendering item.
     * @return Pointer to the current rendering item, or nullptr if none set.
     */
    [[nodiscard]] Rendering::IRenderingItemBase* renderingItem() const noexcept { return m_rendering_item; };

    // =========================================================================
    // Image Operations
    // =========================================================================

    /**
     * @brief Creates the display buffer from a downsampled source image.
     *
     * Called by SourceManager after downsampling. This transfers the image
     * to the rendering item for GPU upload and updates internal state.
     *
     * @param source_image The downsampled image region to display.
     */
    void createDisplayImage(std::unique_ptr<Core::Common::ImageRegion> source_image);

    /**
     * @brief Updates a specific tile region of the display image.
     *
     * Used for progressive loading or tile-based updates.
     *
     * @param source_tile The tile to update in the display.
     */
    void updateDisplayTile(std::unique_ptr<Core::Common::ImageRegion> source_tile);

    /**
     * @brief Sets the source image size metadata.
     *
     * Called when a new source image is loaded. Triggers calculation
     * of optimal display and downsample sizes via ViewportManager.
     *
     * @param width Source image width in pixels.
     * @param height Source image height in pixels.
     */
    Q_INVOKABLE void setSourceImageSize(int width, int height);

    // =========================================================================
    // Zoom & Pan
    // =========================================================================

    /**
     * @brief Sets the zoom level.
     *
     * Zoom is clamped to the range [0.1, 10.0].
     * A value of 1.0 means 100% (no zoom).
     *
     * @param zoom New zoom level.
     */
    Q_INVOKABLE void setZoom(float zoom);

    /**
     * @brief Gets the current zoom level.
     * @return Zoom level (1.0 = 100%).
     */
    [[nodiscard]] float zoom() const noexcept { return m_zoom; };

    /**
     * @brief Sets the pan offset.
     *
     * Pan offset is in logical pixels (QML coordinates).
     *
     * @param pan New pan offset.
     */
    Q_INVOKABLE void setPan(const QPointF& pan);

    /**
     * @brief Gets the current pan offset.
     * @return Pan offset in logical pixels.
     */
    [[nodiscard]] QPointF pan() const noexcept { return m_pan; };

    /**
     * @brief Performs a zoom centered on a specific point.
     *
     * Adjusts pan to keep the specified point under the cursor during zoom.
     * This provides natural "zoom where you point" behavior.
     *
     * @param point Center point in logical coordinates.
     * @param zoom_delta Zoom multiplier (e.g., 1.2 for 20% zoom in).
     */
    Q_INVOKABLE void zoomAt(const QPointF& point, float zoom_delta);

    /**
     * @brief Fits the image to fill the viewport.
     *
     * Calculates the zoom needed to fit the entire image in the viewport
     * and centers it. This is typically called after loading a new image.
     */
    Q_INVOKABLE void fitToView();

    /**
     * @brief Resets the view to default fit state.
     *
     * Equivalent to fitToView().
     */
    Q_INVOKABLE void resetView();

    /**
     * @brief Zooms in by 20%.
     */
    Q_INVOKABLE void zoomIn();

    /**
     * @brief Zooms out by 20% (divides zoom by 1.2).
     */
    Q_INVOKABLE void zoomOut();

    // =========================================================================
    // Viewport
    // =========================================================================

    /**
     * @brief Sets the viewport size.
     *
     * This triggers recalculation of display parameters via ViewportManager.
     * If the MAX_DOWNSAMPLE changes (rare, depends on viewport resize),
     * a new downsampled image may be requested.
     *
     * @param size Viewport size in logical pixels.
     */
    Q_INVOKABLE void setViewportSize(const QSize& size);

    /**
     * @brief Gets the current viewport size.
     * @return Viewport size in logical pixels.
     */
    [[nodiscard]] QSize viewportSize() const noexcept;

    /**
     * @brief Sets the quality margin for zoom headroom.
     *
     * A value of 1.25 means 25% zoom headroom before pixelation.
     * This affects the MAX_DOWNSAMPLE calculation.
     *
     * @param margin Quality margin, clamped to [1.0, 2.0].
     */
    Q_INVOKABLE void setQualityMargin(float margin);

    // =========================================================================
    // Coordinate Mapping
    // =========================================================================

    /**
     * @brief Maps backend (source) coordinates to display coordinates.
     *
     * Converts coordinates from the full-resolution source image space
     * to the display (screen) coordinate space.
     *
     * @param backend_x Backend X coordinate in source pixels.
     * @param backend_y Backend Y coordinate in source pixels.
     * @return Display coordinates in logical pixels.
     */
    Q_INVOKABLE QPointF mapBackendToDisplay(int backend_x, int backend_y) const;

    /**
     * @brief Maps display coordinates to backend (source) coordinates.
     *
     * Converts coordinates from the display (screen) coordinate space
     * to the full-resolution source image space.
     *
     * @param display_x Display X coordinate in logical pixels.
     * @param display_y Display Y coordinate in logical pixels.
     * @return Backend coordinates in source image pixels.
     */
    Q_INVOKABLE QPoint mapDisplayToBackend(float display_x, float display_y) const;

    // =========================================================================
    // Property Getters
    // =========================================================================

    /**
     * @brief Gets the source image size.
     * @return Source image dimensions in pixels.
     */
    [[nodiscard]] QSize sourceImageSize() const noexcept { return m_source_image_size; };
    
    /**
     * @brief Gets the display image size.
     * @return Display image dimensions in logical pixels.
     */
    [[nodiscard]] QSize displayImageSize() const noexcept { return m_display_image_size; };

    /**
     * @brief Gets the downsampled image size (GPU texture size).
     * @return Downsampled dimensions in pixels.
     */
    [[nodiscard]] QSize downsampleSize() const noexcept { return m_downsample_size; };

    /**
     * @brief Gets the display scale factor.
     * @return Scale ratio (display / source).
     */
    [[nodiscard]] float displayScale() const noexcept { return m_display_scale; };

    /**
     * @brief Gets the maximum downsample dimension.
     * @return Maximum dimension in pixels.
     */
    [[nodiscard]] int maxDownsample() const noexcept;

signals:
    /**
     * @brief Emitted when a new downsampled image is needed.
     *
     * Connect this to SourceManager to request a new downsampled version
     * when the display requirements change.
     *
     * @param target_width Requested width in pixels.
     * @param target_height Requested height in pixels.
     */
    void displayImageRequest(int target_width, int target_height);

    /**
     * @brief Emitted when zoom changes.
     * @param zoom New zoom level.
     */
    void zoomChanged(float zoom);

    /**
     * @brief Emitted when pan changes.
     * @param pan New pan offset.
     */
    void panChanged(const QPointF& pan);

    /**
     * @brief Emitted when viewport size changes.
     * @param size New viewport size.
     */
    void viewportSizeChanged(const QSize& size);

    /**
     * @brief Emitted when source image size changes.
     * @param size New source image size.
     */
    void sourceImageSizeChanged(const QSize& size);

    /**
     * @brief Emitted when display image size changes.
     * @param size New display image size.
     */
    void displayImageSizeChanged(const QSize& size);

    /**
     * @brief Emitted when downsample size changes.
     * @param size New downsample size.
     */
    void downsampleSizeChanged(const QSize& size);

    /**
     * @brief Emitted when display scale changes.
     * @param scale New display scale.
     */
    void displayScaleChanged(float scale);

    /**
     * @brief Emitted when max downsample changes.
     * @param max_dim New maximum dimension.
     */
    void maxDownsampleChanged(int max_dim);

    /**
     * @brief Emitted when initialization completes.
     * @param initialized True if successfully initialized.
     */
    void initializedChanged(bool initialized);

private:
    // =========================================================================
    // Internal Methods
    // =========================================================================

    /**
     * @brief Constrains pan to keep image visible in viewport.
     *
     * Called after zoom or pan changes to ensure the image remains
     * at least partially visible.
     */
    void constrainPan();

    /**
     * @brief Calculates display size from source and viewport.
     *
     * Uses ViewportManager for the calculation.
     *
     * @param source_size Source image dimensions.
     * @return Calculated display dimensions.
     */
    [[nodiscard]] QSize calculateDisplaySize(const QSize& source_size) const;

    // =========================================================================
    // Member Variables
    // =========================================================================

    /**
     * @brief Viewport manager for handling viewport and display calculations.
     */
    std::unique_ptr<ViewportManager> m_viewport_manager;

    /** @brief Current zoom level (1.0 = 100%). */
    float m_zoom{1.0f};

    /** @brief Current translation offset in pixels. */
    QPointF m_pan{0, 0};
    
    /** @brief Full resolution dimensions from backend. */
    QSize m_source_image_size;

    /** @brief Target resolution for the display buffer. */
    QSize m_display_image_size;

    /** @brief Calculated downsampled size. */
    QSize m_downsample_size;

    /** @brief Pre-calculated scale factor (m_display_image_size / m_source_image_size). */
    float m_display_scale{1.0f};

    /** @brief Pointer to the active rendering component. */
    Rendering::IRenderingItemBase* m_rendering_item{nullptr};

    /** @brief Cached fit-to-view zoom for the current image. */
    float m_fit_zoom{1.0f};
};

} // namespace Display
} // namespace CaptureMoment::UI
