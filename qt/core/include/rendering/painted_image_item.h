/**
 * @file painted_image_item.h
 * @brief Simple image display using QQuickPaintedItem and QPainter (Qt 6)
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <QQuickPaintedItem>
#include <QMutex>
#include <QPointF>
#include <QImage> // Pour le stockage temporaire de l'image à dessiner
#include <memory>

#include "common/image_region.h"

namespace CaptureMoment::UI {

/**
 * @brief Namespace containing Qt-specific UI components for CaptureMoment.
 * 
 * This namespace includes classes responsible for rendering and UI integration
 * using Qt Quick and potentially the Qt Rendering Hardware Interface (QRhi).
 */
namespace Rendering {

/**
 * @brief QQuickItem that renders an image via QPainter.
 *
 * This class provides a simpler image display component compared to RHIImageItem.
 * It uses QQuickPaintedItem, which relies on QPainter for rendering.
 * It converts ImageRegion data to a format suitable for QPainter (e.g., QImage)
 * and manages zoom and pan operations via painter transformations.
 * This is suitable for basic image display where custom RHI shaders are not needed
 * and ease of implementation is preferred over peak GPU performance for the display itself.
 * The core image *processing* (brightness, etc.) still happens on the CPU via Halide.
 */
class PaintedImageItem : public QQuickPaintedItem {
    Q_OBJECT

private:
    /**
     * @brief QImage representation of the image data to be painted.
     * 
     * This member holds the image data (converted from ImageRegion) ready for QPainter.
     * It's protected by m_image_mutex.
     */
    QImage m_current_qimage;      
    
    /**
     * @brief Mutex protecting access to m_current_qimage and related state.
     * 
     * Ensures thread-safe updates to the image data.
     */
     mutable QMutex m_image_mutex;

    // Zoom/Pan
    /**
     * @brief Current zoom level applied to the image.
     * 
     * A value of 1.0f represents the original size.
     */
    float m_zoom{1.0f};
    /**
     * @brief Current pan offset applied to the image.
     * 
     * Represents the offset in scene coordinates.
     */
    QPointF m_pan{0, 0};
    
    // Image metadata
    /**
     * @brief Width of the currently loaded image in pixels.
     */
    int m_image_width{0};
    /**
     * @brief Height of the currently loaded image in pixels.
     */
    int m_image_height{0};

public:
    /**
     * @brief Constructs a new PaintedImageItem.
     * @param parent The parent QQuickItem, if any.
     */
    explicit PaintedImageItem(QQuickItem* parent = nullptr);
    
    /**
     * @brief Destroys the PaintedImageItem.
     */
    ~PaintedImageItem() = default;
    
    /**
     * @brief Sets the full image to be displayed.
     * 
     * This method safely updates the internal image data by converting the ImageRegion
     * to a QImage and scheduling a repaint via update().
     * 
     * @param image A shared pointer to the ImageRegion containing the full-resolution image data.
     */
    void setImage(const std::shared_ptr<ImageRegion>& image);
    
    /**
     * @brief Updates a specific tile of the displayed image.
     * 
     * This method merges the data from the provided tile into the internal QImage
     * buffer and schedules a repaint via update(). It's intended for incremental updates
     * after processing specific regions.
     * 
     * @param tile A shared pointer to the ImageRegion containing the processed tile data.
     */
    void updateTile(const std::shared_ptr<ImageRegion>& tile);
    
    // Zoom/Pan
    /**
     * @brief Sets the zoom level.
     * @param zoom The new zoom factor (e.g., 1.0f for original size).
     */
    void setZoom(float zoom);
    /**
     * @brief Gets the current zoom level.
     * @return The current zoom factor.
     */
    float zoom() const { return m_zoom; }
    /**
     * @brief Sets the pan offset.
     * @param pan The new pan offset as a QPointF.
     */
    void setPan(const QPointF& pan);
    /**
     * @brief Gets the current pan offset.
     * @return The current pan offset.
     */
    QPointF pan() const { return m_pan; }

    /**
     * @brief Get the width of the image.
     * @return The image width in pixels, or 0 if no image is loaded.
     */
    int imageWidth() const;
    /**
     * @brief Get the height of the image.
     * @return The image height in pixels, or 0 if no image is loaded.
     */
    int imageHeight() const;

signals:
    /**
     * @brief Signal emitted when the zoom value changes.
     * @param zoom The new zoom factor.
     */
    void zoomChanged(float zoom);
    /**
     * @brief Signal emitted when the pan offset changes.
     * @param pan The new pan offset.
     */
    void panChanged(const QPointF& pan);
    /**
     * @brief Signal emitted when the image dimensions change (width or height).
     */
    void imageDimensionsChanged();

protected:
    // QQuickPaintedItem override
    /**
     * @brief Paints the image using QPainter.
     * 
     * This override is called by the Qt Quick scene graph to render the item's content.
     * It uses QPainter to draw the internal m_current_qimage, applying zoom and pan
     * transformations.
     * 
     * @param painter The QPainter instance to use for drawing.
     */
    void paint(QPainter* painter) override;
    
private:
    /**
     * @brief Converts an ImageRegion to a QImage.
     * 
     * This helper function converts the internal ImageRegion (float32) data
     * to a format suitable for QPainter (QImage, uint8).
     * @param region The ImageRegion to convert.
     * @return The resulting QImage.
     */
    QImage convertImageRegionToQImage(const ImageRegion& region) const;
};

} // namespace Rendering

} // namespace CaptureMoment::UI
