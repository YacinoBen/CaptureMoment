/**
 * @file rhi_image_item.h
 * @brief Modern RHI-based image display (Qt 6 - Vulkan/Metal/DX12 compatible)
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <QQuickItem>
#include <QMutex>
#include <QSGNode>
#include <memory>

#include "rendering/i_rendering_item_base.h"

namespace CaptureMoment::UI {

/**
 * @brief Namespace containing Qt-specific UI components for CaptureMoment.
 * 
 * This namespace includes classes responsible for rendering and UI integration
 * using Qt Quick and the Qt Rendering Hardware Interface (QRhi).
 */
namespace Rendering {

class RHIImageNode;

/**
 * @class RHIImageItem
 * @brief Modern QQuickItem that renders an image via RHI (Vulkan/Metal/DX12/OpenGL).
 * 
 * This class provides a high-performance image display component compatible with all
 * graphics APIs supported by QRhi. It uses QSGRenderNode for direct RHI access,
 * allowing efficient GPU-based rendering of image data stored in ImageRegion objects.
 * 
 * It manages the GPU texture, handles zoom and pan operations, and provides methods
 * to update the displayed image or specific tiles of it.
 */

class RHIImageItem : public QQuickItem, public IRenderingItemBase {
    Q_OBJECT

private:     
    /**
     * @brief Flag indicating if the GPU texture needs to be updated from m_full_image.
     * 
     * Set to true when setImage or updateTile is called to signal the render node.
     */
    bool m_texture_needs_update{false};

    /**
     * @brief Mutex protecting access to m_full_image and related state.
     * 
     * Ensures thread-safe updates to the image data.
     */
    QMutex m_image_mutex;

public:
    /**
     * @brief Constructs a new RHIImageItem.
     * @param parent The parent QQuickItem, if any.
     */
    explicit RHIImageItem(QQuickItem* parent = nullptr);
    /**
     * @brief Destroys the RHIImageItem and releases associated resources.
     */
    ~RHIImageItem();
    
    /**
     * @brief Sets the full image to be displayed.
     * 
     * This method safely updates the internal image data and marks the GPU texture
     * for an update on the next render pass.
     * 
     * @param image A shared pointer to the ImageRegion containing the full-resolution image data.
     */
    void setImage(const std::shared_ptr<ImageRegion>& image) override;
    
    /**
     * @brief Updates a specific tile of the displayed image.
     * 
     * This method merges the data from the provided tile into the full image buffer
     * and marks the GPU texture for an update. It's intended for incremental updates
     * after processing specific regions.
     * 
     * @param tile A shared pointer to the ImageRegion containing the processed tile data.
     */
    void updateTile(const std::shared_ptr<ImageRegion>& tile) override;
    
    // Zoom/Pan
    /**
     * @brief Sets the zoom level.
     * @param zoom The new zoom factor (e.g., 1.0f for original size).
     */
    void setZoom(float zoom) override;
    /**
     * @brief Sets the pan offset.
     * @param pan The new pan offset as a QPointF.
     */
    void setPan(const QPointF& pan) override;

    /**
     * @brief Get the zoom level.
     */
    float zoom() const { return m_zoom; };
   
    /**
     * @brief Get the pan offset.
     */
    QPointF pan() const { return m_pan; };

    /**
     * @brief Get the width of the image.
     */
    int imageWidth() const { return m_image_width; };

    /**
     * @brief Get the height of the image.
     */
    int imageHeight() const { return m_image_height; };

signals:
    /**
     * @brief Signal emitted when the zoom change.
     */
    void zoomChanged(float zoom);

    /**
     * @brief Signal emitted when the pan change.
     */
    void panChanged(const QPointF& pan);

    /**
     * @brief Signal emitted when the image dimensions change (width or height).
     */
    void imageSizeChanged();

protected:
    // QQuickItem overrides
    /**
     * @brief Updates the scene graph node for this item.
     * 
     * This override creates and returns the QSGRenderNode responsible for
     * rendering the image using QRhi.
     * 
     * @param node The previous QSGNode, if any.
     * @param data Update data provided by the scene graph.
     * @return The QSGRenderNode instance for this item.
     */
    QSGNode* updatePaintNode(QSGNode* node, UpdatePaintNodeData* data) override;
    
private:

    friend class RHIImageNode; 
};

} // namespace Rendering

} // namespace CaptureMoment::Qt
