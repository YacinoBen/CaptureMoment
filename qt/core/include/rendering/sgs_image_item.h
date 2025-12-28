/**
 * @file sgs_image_item.h
 * @brief Simple texture-based image display (Qt6 - QSGSimpleTextureNode)
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <QQuickItem>
#include <QMutex>
#include <QSGNode>
#include <QSGTexture>
#include <QSizeF>
#include <QPointF>

#include "rendering/base_image_item.h"

namespace CaptureMoment::UI {

/**
 * @brief Namespace containing Qt-specific UI components for CaptureMoment.
 *
 * This namespace includes classes responsible for rendering and UI integration
 * using Qt Quick and potentially the Qt Rendering Hardware Interface (QRhi).
 */
namespace Rendering {

/**
 * @brief QQuickItem that renders an image via QSGSimpleTextureNode.
 *
 * This class provides an image display component using the simpler QSGSimpleTextureNode
 * instead of the more complex QSGRenderNode. It converts Core::Common::ImageRegion data to a format
 * suitable for QSGTexture (e.g., QImage) and manages zoom and pan operations.
 * It's a good choice for basic image display where custom RHI shaders are not needed.
 * Inherits from IRenderingItemBase to manage common state (zoom, pan, image dimensions).
 */

class SGSImageItem : public QQuickItem, public BaseImageItem{
    Q_OBJECT

private:
    /**
     * @brief Cached QSGTexture representing the image on the GPU.
     *
     * This texture is created/updated from m_full_image when m_texture_needs_update is true.
     */
    QSGTexture* m_cached_texture {nullptr};

     /**
     * @brief Mutex protecting access to m_full_image and related state.
     *
     * Ensures thread-safe updates to the image data.
     */
     mutable QMutex m_image_mutex;

     /**
     * @brief Temporary texture created on the main thread, to be transferred to the render thread.
     *
     * This member holds the newly created GPU texture after conversion on the main thread.
     * It is accessed by the main thread for creation and by the render thread for consumption.
     * Protected by m_pending_mutex.
     */
     QSGTexture* m_pending_texture{nullptr};

     /**
     * @brief Mutex protecting access to m_pending_texture.
     *
     * Ensures thread-safe transfer of the new texture from the main thread to the render thread.
     */
     mutable QMutex m_pending_mutex;

public:
    /**
     * @brief Constructs a new SGSImageItem.
     * @param parent The parent QQuickItem, if any.
     */
    explicit SGSImageItem(QQuickItem* parent = nullptr);

    /**
     * @brief Destroys the SGSImageItem and releases associated resources.
     */
    ~SGSImageItem() override;

    /**
     * @brief Sets the full image to be displayed.
     *
     * This method safely updates the internal image data and marks the GPU texture
     * for an update on the next render pass.
     * 
     * @param image A shared pointer to the Core::Common::ImageRegion containing the full-resolution image data.
     */
    void setImage(const std::shared_ptr<Core::Common::ImageRegion>& image) override;
            
    /**
     * @brief Updates a specific tile of the displayed image.
     *
     * This method merges the data from the provided tile into the full image buffer
     * and marks the GPU texture for an update. It's intended for incremental updates
     * after processing specific regions.
     * 
     * @param tile A shared pointer to the Core::Common::ImageRegion containing the processed tile data.
     */
    void updateTile(const std::shared_ptr<Core::Common::ImageRegion>& tile) override;
            
    // Zoom/Pan
    /**
     * @brief Sets the zoom level.
     * @param zoom The new zoom factor (e.g., 1.0f for original size).
     */
    void setZoom(float zoom) override; //call only Update

    /**
     * @brief Sets the pan offset.
     * @param pan The new pan offset as a QPointF.
     */
    void setPan(const QPointF& pan) override;  // call only Update


protected:
    // QQuickItem overrides
    /**
     * @brief Updates the scene graph node for this item.
     *
     * This override creates and returns the QSGNode responsible for
     * rendering the image using QSGSimpleTextureNode.
     *
     * @param node The previous QSGNode, if any.
     * @param data Update data provided by the scene graph.
     * @return The QSGNode instance for this item.
     */
    QSGNode* updatePaintNode(QSGNode* node, UpdatePaintNodeData* data) override;

private:
    /**
     * @brief Converts the internal Core::Common::ImageRegion to a QSGTexture.
     * 
     * This helper function converts the m_full_image (float32) to a QImage (uint8),
     * then creates the m_cached_texture used by the QSGSimpleTextureNode.
     * This function runs on the main thread.
     */
    void updateTextureOnMainThread();

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
    void imageSizeChanged();
};

} // namespace Rendering

} // namespace CaptureMoment::UI
