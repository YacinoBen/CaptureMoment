/**
 * @file rhi_image_item.h
 * @brief Modern RHI-based image display (Qt6 - Vulkan/Metal/DX12 compatible) using QQuickRhiItem.
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
 * management (zoom, pan, mutex-protected image storage).
 *
 * @par Threading Model
 * - GUI Thread: RHIImageItem lives here, receives setImage() calls
 * - Render Thread: RHIImageItemRenderer lives here, performs GPU operations
 * - Synchronization: synchronize() copies data between threads safely via mutex
 *
 * @par Usage Example
 * @code
 * // From QML
 * RHIImageItem {
 *     id: imageDisplay
 *     anchors.fill: parent
 * }
 *
 * // From C++
 * auto item = new RHIImageItem();
 * item->setImage(std::make_unique<ImageRegion>(data, width, height, 4));
 * @endcode
 *
 * @see RHIImageItemRenderer for the GPU rendering implementation
 * @see BaseImageItem for common state management
 * @see QQuickRhiItem for Qt's RHI integration
 */
class RHIImageItem : public QQuickRhiItem, public BaseImageItem {
    Q_OBJECT

private:
    /**
     * @brief Flag indicating if the GPU texture needs to be updated from m_full_image.
     *
     * Set to true when setImage or updateTile is called to signal the render thread.
     * This flag is accessed from both GUI and render threads, protected by m_image_mutex.
     */
    bool m_texture_needs_update{false};

    friend class RHIImageItemRenderer;

public:
    /**
     * @brief Constructs a new RHIImageItem.
     *
     * Initializes the item with default zoom (1.0) and pan (0, 0).
     * The renderer will be created lazily when the item becomes visible.
     *
     * @param parent The parent QQuickItem, if any.
     */
    explicit RHIImageItem(QQuickItem* parent = nullptr);

    /**
     * @brief Destroys the RHIImageItem and releases associated resources.
     *
     * The RHIImageItemRenderer is automatically destroyed by Qt's scene graph.
     * CPU-side image data is released via unique_ptr destructor.
     */
    ~RHIImageItem() override;

    /**
     * @brief Sets the full image to be displayed.
     *
     * This method safely updates the internal image data and marks the GPU texture
     * for an update on the next render pass. The texture update happens in the render
     * thread during the synchronize() call.
     *
     * @param image The image data as an ImageRegion. Ownership is transferred.
     *
     * @pre image != nullptr
     * @pre image->isValid() == true
     * @post m_full_image contains the new image
     * @post m_texture_needs_update is set to true
     *
     * @note Thread-safe: uses m_image_mutex
     * @note Called from GUI thread
     */
    void setImage(std::unique_ptr<Core::Common::ImageRegion> image) override;

    /**
     * @brief Updates a specific tile of the displayed image.
     *
     * This method merges the data from the provided tile into the full image buffer
     * and marks the GPU texture for an update. It's intended for incremental updates
     * after processing specific regions.
     *
     * @param tile The image tile containing the updated region. Ownership is transferred.
     *
     * @pre tile != nullptr
     * @pre tile->isValid() == true
     * @pre m_full_image != nullptr (base image must exist)
     *
     * @note Thread-safe: uses m_image_mutex
     * @note Called from GUI thread
     */
    void updateTile(std::unique_ptr<Core::Common::ImageRegion> tile) override;

    /**
     * @brief Gets the state of the texture update flag.
     *
     * This method provides access to the flag indicating if the GPU texture
     * needs to be updated from the internal image data.
     *
     * @return True if the texture needs an update, false otherwise.
     *
     * @note Thread-safe: uses m_image_mutex
     */
    [[nodiscard]] bool textureNeedsUpdate() const;

    /**
     * @brief Sets the state of the texture update flag.
     *
     * This method allows setting the flag indicating if the GPU texture
     * needs to be updated. Typically called from the render thread during
     * synchronize() after texture upload is complete.
     *
     * @param needs_update The new state of the flag.
     *
     * @note Thread-safe: uses m_image_mutex
     */
    void setTextureNeedsUpdate(bool needs_update);

signals:
    /**
     * @brief Signal emitted when the zoom value changes.
     * @param zoom The new zoom factor.
     */
    void zoomChanged(float zoom);

    /**
     * @brief Signal emitted when the pan offset changes.
     * @param pan The new pan offset in image coordinates.
     */
    void panChanged(const QPointF& pan);

    /**
     * @brief Signal emitted when the image dimensions change (width or height).
     *
     * This signal is emitted after a successful setImage() call with
     * different dimensions than the previous image.
     */
    void imageSizeChanged();

protected:
    // =========================================================================
    // QQuickRhiItem Interface
    // =========================================================================

    /**
     * @brief Creates the renderer instance responsible for RHI rendering.
     *
     * This override creates and returns a new instance of RHIImageItemRenderer
     * which handles the actual GPU rendering commands. Called by Qt's scene graph
     * when the item needs to be rendered for the first time.
     *
     * @return A new instance of RHIImageItemRenderer. Ownership is transferred to Qt.
     *
     * @note Called on the render thread
     */
    QQuickRhiItemRenderer *createRenderer() override;

    // =========================================================================
    // BaseImageItem Virtual Handlers
    // =========================================================================

    /**
     * @brief Handler called when zoom changes.
     *
     * Emits the zoomChanged signal and requests an update.
     *
     * @param new_zoom The new zoom factor.
     */
    void onZoomChanged(float new_zoom) override;

    /**
     * @brief Handler called when pan changes.
     *
     * Emits the panChanged signal and requests an update.
     *
     * @param new_pan The new pan offset.
     */
    void onPanChanged(const QPointF& new_pan) override;

    /**
     * @brief Handler called when image data changes.
     *
     * Emits the imageSizeChanged signal and requests an update.
     */
    void onImageChanged() override;
};

} // namespace Rendering

} // namespace CaptureMoment::UI
