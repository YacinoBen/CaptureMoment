/**
 * @file rhi_image_item.h
 * @brief Modern RHI-based image display (Qt 6 - Vulkan/Metal/DX12 compatible) using QQuickRhiItem.
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <QQuickRhiItem>
#include <QMutex>
#include <memory>

#include "rendering/base_image_item.h"

namespace CaptureMoment::UI {

/**
 * @brief Namespace containing Qt-specific UI components for CaptureMoment.
 *
 * This namespace includes classes responsible for rendering and UI integration
 * using Qt Quick and the Qt Rendering Hardware Interface (QRhi).
 */
namespace Rendering {

class RHIImageItemRenderer; // Forward declaration

/**
 * @class RHIImageItem
 * @brief Modern QQuickItem that renders an image via RHI (Vulkan/Metal/DX12/OpenGL) using QQuickRhiItem.
 *
 * This class provides a high-performance image display component compatible with all
 * graphics APIs supported by QRhi. It uses QQuickRhiItem to integrate RHI rendering
 * into the Qt Quick scene graph, allowing efficient GPU-based rendering of image data
 * stored in ImageRegion objects.
 *
 * It manages the CPU-side image data, handles zoom and pan operations, and provides methods
 * to update the displayed image or specific tiles of it. The actual GPU rendering is handled
 * by the RHIImageItemRenderer class.
 *
 * This class inherits from QQuickRhiItem for RHI integration and BaseImageItem for common state
 */
class RHIImageItem : public QQuickRhiItem, public BaseImageItem { // Hérite de QQuickRhiItem et BaseImageItem
    Q_OBJECT

private:
    /**
     * @brief Flag indicating if the GPU texture needs to be updated from m_full_image.
     *
     * Set to true when setImage or updateTile is called to signal the render thread.
     */
    bool m_texture_needs_update{false};

public:
    /**
     * @brief Constructs a new RHIImageItem.
     * @param parent The parent QQuickItem, if any.
     */
    explicit RHIImageItem(QQuickItem* parent = nullptr);

    /**
     * @brief Destroys the RHIImageItem and releases associated resources.
     */
    ~RHIImageItem() override;

    /**
     * @brief Sets the full image to be displayed.
     *
     * This method safely updates the internal image data and marks the GPU texture
     * for an update on the next render pass. The texture update happens in the render thread.
     *
     * @param image A shared pointer to the ImageRegion containing the full-resolution image data.
     */
    void setImage(const std::shared_ptr<Core::Common::ImageRegion>& image) override;

    /**
     * @brief Updates a specific tile of the displayed image.
     *
     * This method merges the data from the provided tile into the full image buffer
     * and marks the GPU texture for an update. It's intended for incremental updates
     * after processing specific regions. The texture update happens in the render thread.
     *
     * @param tile A shared pointer to the ImageRegion containing the processed tile data.
     */
    void updateTile(const std::shared_ptr<Core::Common::ImageRegion>& tile) override;

    /**
     * @brief Gets the state of the texture update flag.
     *
     * This method provides access to the flag indicating if the GPU texture
     * needs to be updated from the internal image data.
     *
     * @return True if the texture needs an update, false otherwise.
     */
    [[nodiscard]] bool textureNeedsUpdate() const;

    /**
     * @brief Sets the state of the texture update flag.
     *
     * This method allows setting the flag indicating if the GPU texture
     * needs to be updated. It should be called from the main thread.
     *
     * @param update The new state of the flag.
     */
    void setTextureNeedsUpdate(bool update);

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
    // QQuickRhiItem overrides
    /**
     * @brief Creates the renderer instance responsible for RHI rendering.
     *
     * This override creates and returns a new instance of RHIImageItemRenderer
     * which handles the actual GPU rendering commands.
     *
     * @return A new instance of RHIImageItemRenderer.
     */
    QQuickRhiItemRenderer *createRenderer() override;
};

} // namespace Rendering

} // namespace CaptureMoment::UI
