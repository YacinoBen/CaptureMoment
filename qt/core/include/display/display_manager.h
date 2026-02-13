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
 * @brief Manages the display representation of the image.
 * * This class acts as an intermediary between:
 * - Backend: Full resolution image (operations work here)
 * - Frontend: Downsampled image for display (what user sees)
 * * Flow:
 * SourceManager (4K) -> DisplayManager (downsample) -> IRenderingItemBase (display)
 */
class DisplayManager : public QObject {
    Q_OBJECT
    
    /** @property zoom The current magnification level of the displayed image. */
    Q_PROPERTY(float zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    /** @property pan The current translation offset of the displayed image. */
    Q_PROPERTY(QPointF pan READ pan WRITE setPan NOTIFY panChanged)
    /** @property viewportSize The size of the UI container area. */
    Q_PROPERTY(QSize viewportSize READ viewportSize WRITE setViewportSize NOTIFY viewportSizeChanged)
    /** @property sourceImageSize The resolution of the original backend image. */
    Q_PROPERTY(QSize sourceImageSize READ sourceImageSize NOTIFY sourceImageSizeChanged)
    /** @property displayImageSize The resolution of the downsampled buffer. */
    Q_PROPERTY(QSize displayImageSize READ displayImageSize NOTIFY displayImageSizeChanged)
    /** @property displayScale Ratio between display and source dimensions. */
    Q_PROPERTY(float displayScale READ displayScale NOTIFY displayScaleChanged)
    
private:
    /** @brief Current zoom level (1.0 = 100%). */
    float m_zoom{1.0f};

    /** @brief Current translation offset in pixels. */
    QPointF m_pan{0, 0};

    /** @brief Dimensions of the visible UI area. */
    QSize m_viewport_size{800, 600};
    
    /** @brief Full resolution dimensions from backend. */
    QSize m_source_image_size;

    /** @brief Target resolution for the display buffer. */
    QSize m_display_image_size;

    /** @brief Pre-calculated scale factor (m_display_image_size / m_source_image_size). */
    float m_display_scale{1.0f};

    /** @brief Pointer to the active rendering component. */
    Rendering::IRenderingItemBase* m_rendering_item{nullptr};


    /**
     * @brief Shared pointer to the High-Res Source Image.
     * DisplayManager does NOT own this memory, but holds a reference
     * to allow re-downsampling on Viewport resize without querying Controller.
     */
    std::shared_ptr<Core::Common::ImageRegion> m_source_image;

public:
    /**
     * @brief Constructs a DisplayManager.
     * @param parent Optional Qt parent object.
     */
    explicit DisplayManager(QObject* parent = nullptr);
    
    /** @brief Default destructor. */
    ~DisplayManager() = default;
    
    /**
     * @brief Assigns the rendering item that will receive image updates.
     * @param item Pointer to a Rendering::IRenderingItemBase implementation.
     */
    void setRenderingItem(Rendering::IRenderingItemBase* item);

    /** @return Pointer to the current rendering item. */
    Rendering::IRenderingItemBase* renderingItem() const { return m_rendering_item; }
    
    /**
     * @brief Initializes the display buffer from a full-resolution source.
     * @param sourceImage Shared pointer to the source ImageRegion.
     */
    void createDisplayImage(const std::shared_ptr<Core::Common::ImageRegion>& sourceImage);
    
    /**
     * @brief Updates a specific part of the display image.
     * @param sourceTile The high-resolution tile to be downsampled and updated.
     */
    void updateDisplayTile(const std::shared_ptr<Core::Common::ImageRegion>& sourceTile);
    
    /**
     * @brief Updates the magnification level.
     * @param zoom The new zoom factor.
     */
    void setZoom(float zoom);
    
    /** @return The current zoom factor. */
    float zoom() const { return m_zoom; }
    
    /**
     * @brief Updates the translation offset.
     * @param pan The new pan coordinates.
     */
    void setPan(const QPointF& pan);
    
    /** @return The current pan offset. */
    QPointF pan() const { return m_pan; }
    
    /**
     * @brief Performs a zoom centered on a specific point.
     * @param point The point (usually cursor position) to zoom toward.
     * @param zoom_delta The multiplier to apply to current zoom.
     */
    Q_INVOKABLE void zoomAt(const QPointF& point, float zoom_delta);
    
    /** @brief Adjusts zoom and pan to fit the entire image within the viewport. */
    Q_INVOKABLE void fitToView();
    
    /** @brief Resets zoom to 1.0 and centers the view. */
    Q_INVOKABLE void resetView();
    
    /** @brief Incremental zoom in. */
    Q_INVOKABLE void zoomIn() { setZoom(m_zoom * 1.2f); }
    
    /** @brief Incremental zoom out. */
    Q_INVOKABLE void zoomOut() { setZoom(m_zoom / 1.2f); }
    
    /**
     * @brief Updates the viewport dimensions.
     * @param size New size of the viewport.
     */
    Q_INVOKABLE void setViewportSize(const QSize& size);
    
    /** @return The current viewport size. */
    QSize viewportSize() const { return m_viewport_size; }
    
    /**
     * @brief Maps backend (full-res) coordinates to screen coordinates.
     * @param backend_x x coordinate in the source image.
     * @param backend_y y coordinate in the source image.
     * @return Transformed coordinates on the display.
     */
    Q_INVOKABLE QPointF mapBackendToDisplay(int backend_x, int backend_y) const;
    
    /**
     * @brief Maps screen coordinates back to backend (full-res) coordinates.
     * @param display_x x coordinate on the display.
     * @param display_y y coordinate on the display.
     * @return Corresponding coordinates in the source image.
     */
    Q_INVOKABLE QPoint mapDisplayToBackend(float display_x, float display_y) const;
    
    /** @return The original source image size. */
    QSize sourceImageSize() const { return m_source_image_size; }
    
    /** @return The internal downsampled buffer size. */
    QSize displayImageSize() const { return m_display_image_size; }
    
    /** @return The scaling ratio between source and display. */
    float displayScale() const { return m_display_scale; }

signals:
    /** @brief Emitted when the zoom level changes. */
    void zoomChanged(float zoom);
    /** @brief Emitted when the pan offset changes. */
    void panChanged(const QPointF& pan);
    /** @brief Emitted when the viewport size is updated. */
    void viewportSizeChanged(const QSize& size);
    /** @brief Emitted when a new source image size is detected. */
    void sourceImageSizeChanged(const QSize& size);
    /** @brief Emitted when the display buffer size is calculated. */
    void displayImageSizeChanged(const QSize& size);
    /** @brief Emitted when the internal display scale changes. */
    void displayScaleChanged(float scale);
    
private:
    /**
     * @brief Determines the optimal downsampled size based on viewport and source.
     * @param sourceSize Original dimensions.
     * @param viewportSize UI container dimensions.
     * @return Calculated display dimensions.
     */
    QSize calculateDisplaySize(const QSize& source_size, const QSize& viewport_size) const;
    
    /**
     * @brief Performs the actual downsampling logic.
     * @param source Source ImageRegion data.
     * @param target_width Desired width.
     * @param target_height Desired height.
     * @return A new downsampled ImageRegion.
     */
    std::shared_ptr<Core::Common::ImageRegion> downsampleImage(
        const Core::Common::ImageRegion& source, int targetWidth, int targetHeight
    ) const;
    
    /** @brief Ensures the pan offset keeps the image within reasonable bounds. */
    void constrainPan();
};

} // namespace Display

} // namespace CaptureMoment::UI

