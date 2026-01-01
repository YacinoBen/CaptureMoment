/**
 * @file sgs_image_item.h
 * @brief Simple texture-based image display (Qt6 - QSGSimpleTextureNode) using BaseImageItem for common state.
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

#include "rendering/base_image_item.h" // Inherits from BaseImageItem and IRenderingItemBase

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
 * Inherits from QQuickItem for Qt Quick integration and BaseImageItem for common state
 * (zoom, pan, image dimensions) and QML properties/signals.
 */
class SGSImageItem : public QQuickItem, public BaseImageItem { // Hérite des deux
    Q_OBJECT

private:
    /**
     * @brief Mutex protecting access to m_full_image and related state.
     *
     * Ensures thread-safe updates to the image data (from the main thread)
     * and safe reading during the rendering process (on the render thread).
     */
    mutable QMutex m_image_mutex;

    /**
     * @brief Flag indicating if the internal image data has changed and needs conversion.
     *
     * Set to true when setImage or updateTile is called. Checked in updatePaintNode
     * to determine if a new QImage and QSGTexture need to be created.
     */
    bool m_image_dirty{false};

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
     * This method safely updates the internal image data and marks the internal state
     * for an update on the next render pass. The conversion to GPU texture happens
     * in updatePaintNode on the render thread.
     *
     * @param image A shared pointer to the ImageRegion containing the full-resolution image data.
     */
    void setImage(const std::shared_ptr<Core::Common::ImageRegion>& image) override;
            
    /**
     * @brief Updates a specific tile of the displayed image.
     *
     * This method safely merges the data from the provided tile into the internal
     * full image buffer (CPU side) and marks the internal state for an update.
     * The conversion to GPU texture happens in updatePaintNode on the render thread.
     *
     * @param tile A shared pointer to the ImageRegion containing the processed tile data.
     */
    void updateTile(const std::shared_ptr<Core::Common::ImageRegion>& tile) override;

    // Zoom/Pan (Implementation provided by BaseImageItem, setters implemented here)
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

protected:
    // QQuickItem overrides
    /**
     * @brief Updates the scene graph node for this item.
     *
     * This override creates and returns the QSGNode responsible for
     * rendering the image using QSGSimpleTextureNode.
     * It handles the conversion from ImageRegion to QImage to QSGTexture on the render thread.
     *
     * @param node The previous QSGNode, if any.
     * @param data Update data provided by the scene graph.
     * @return The QSGNode instance for this item.
     */
    QSGNode* updatePaintNode(QSGNode* node, UpdatePaintNodeData* data) override;
};

} // namespace Rendering

} // namespace CaptureMoment::UI
