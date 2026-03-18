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
 * Inherits from QQuickItem for Qt Quick integration and BaseImageItem for common state
 * (zoom, pan, image dimensions) and QML properties/signals.
 */
class SGSImageItem : public QQuickItem, public BaseImageItem {
    Q_OBJECT

private:
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
     * @param image The image data.
     */
    void setImage(std::unique_ptr<Core::Common::ImageRegion> image) override;

    /**
     * @brief Updates a specific tile of the displayed image.
     *
     * This method safely merges the data from the provided tile into the internal
     * full image buffer (CPU side) and marks the internal state for an update.
     * The conversion to GPU texture happens in updatePaintNode on the render thread.
     *
     * @param tile The image tile containing the updated region.
     */
    void updateTile(std::unique_ptr<Core::Common::ImageRegion> tile) override;

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

    /*
     * @brief Handlers for state changes (zoom, pan, image).
     * These methods are called when the corresponding state changes (zoom, pan, image).
     * They can be overridden to react to changes, but the base implementation does nothing.
     */
    void onZoomChanged(float new_zoom) override;

    /*
     * @brief Handler for pan changes.
     * This method is called when the pan offset changes.
     * The base implementation does nothing, but it can be overridden to react to pan changes.
     */
    void onPanChanged(const QPointF& new_pan) override;

    /*
     * @brief Handler for image changes.
     * This method is called when the image data changes (setImage or updateTile).
     * The base implementation does nothing, but it can be overridden to react to image changes.
     */
    void onImageChanged() override;
};

} // namespace Rendering

} // namespace CaptureMoment::UI
