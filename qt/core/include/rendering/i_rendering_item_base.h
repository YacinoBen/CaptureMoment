/**
 * @file rendering_item_base.h
 * @brief Abstract base class for image display items managing common state (zoom, pan, dimensions).
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <QObject>
#include <QMutex>
#include <QPointF>
#include <string>

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
 * @brief Abstract base class for image display items managing common state (zoom, pan, dimensions).
 *
 * This class provides a common interface and shared state management for image display items
 * like PaintedImageItem, SGSImageItem, or RHIImageItem. It encapsulates the logic for
 * zoom, pan, image dimensions, and the basic setImage/updateTile methods.
 * It does not inherit from QQuickItem or any other Qt Quick base class, allowing
 * different rendering backends (QPainter, QSGTexture, QRhi) to be used by derived classes.
 */
class IRenderingItemBase {
protected:
    /**
     * @brief Shared pointer to the full image data displayed by this item.
     * 
     * This member holds the complete image data (CPU side, float32) which is used to update the display.
     */
    std::shared_ptr<Core::Common::ImageRegion> m_full_image;
    
    /**
     * @brief Flag indicating if the display needs to be updated from m_full_image.
     * 
     * Set to true when setImage or updateTile is called to signal the render node.
     */
    bool m_display_needs_update{false};

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
     * @brief Virtual destructor for safe inheritance.
     */
    virtual ~IRenderingItemBase() = default;
    
    /**
     * @brief Sets the full image to be displayed.
     * 
     * This method safely updates the internal image data and marks the display
     * for an update on the next render pass.
     * 
     * @param image A shared pointer to the ImageRegion containing the full-resolution image data.
     */
    virtual void setImage(const std::shared_ptr<Core::Common::ImageRegion>& image) = 0;
    
    /**
     * @brief Updates a specific tile of the displayed image.
     * 
     * This method merges the data from the provided tile into the internal image buffer
     * and marks the display for an update. It's intended for incremental updates
     * after processing specific regions.
     * 
     * @param tile A shared pointer to the ImageRegion containing the processed tile data.
     */
    virtual void updateTile(const std::shared_ptr<Core::Common::ImageRegion>& tile) = 0;
    
    // Zoom/Pan
    /**
     * @brief Sets the zoom level.
     * @param zoom The new zoom factor (e.g., 1.0f for original size).
     */
    virtual void setZoom(float zoom) = 0;
    /**
     * @brief Gets the current zoom level.
     * @return The current zoom factor.
     */
    virtual float zoom() const = 0;
    /**
     * @brief Sets the pan offset.
     * @param pan The new pan offset as a QPointF.
     */
    virtual void setPan(const QPointF& pan) = 0;
    /**
     * @brief Gets the current pan offset.
     * @return The current pan offset.
     */
    virtual QPointF pan() const = 0;

    /**
     * @brief Get the width of the image.
     * @return The image width in pixels, or 0 if no image is loaded.
     */
    virtual int imageWidth() const = 0;
    /**
     * @brief Get the height of the image.
     * @return The image height in pixels, or 0 if no image is loaded.
     */
    virtual int imageHeight() const = 0;
};

} // namespace Rendering

} // namespace CaptureMoment::UI
