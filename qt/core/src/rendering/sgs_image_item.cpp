/**
 * @file sgs_image_item.cpp
 * @brief Implementation of SGSImageItem using QSGSimpleTextureNode
 * @author CaptureMoment Team
 * @date 2025
 */

#include "rendering/sgs_image_item.h"
#include <spdlog/spdlog.h>
#include <QMutexLocker>
#include <QSGSimpleTextureNode>
#include <QSGTexture>
#include <QQuickWindow>
#include <QImage>
#include <algorithm>
#include <cstring>

namespace CaptureMoment::UI::Rendering {

// Constructor: Initializes the item and sets the flag for custom content.
SGSImageItem::SGSImageItem(QQuickItem* parent)
{
    // Indicate to Qt Quick that this item has custom content rendered via the scene graph.
    setFlag(QQuickItem::ItemHasContents, true);
    setSize(QSizeF(800, 600)); // Taille par défaut
    spdlog::info("SGSImageItem: Created");
}

// Destructor: Cleans up resources.
SGSImageItem::~SGSImageItem() {
    // The removal of m_cached_texture is done in updateCachedTexture

    // or in the QSGNode destructor if it is attached to the node.

    // Here we ensure that the rendering thread no longer uses the texture.

    // This is often done automatically via Qt Quick mechanisms.

    // delete m_cached_texture; // <-- DANGEROUS here if the texture is attached to the rendering thread node
    spdlog::debug("SGSImageItem: Destroyed");
}

    // Sets the full image to be displayed.
    // Updates the internal image data and marks the GPU texture for update.
    void SGSImageItem::setImage(const std::shared_ptr<Core::Common::ImageRegion>& image)
    {
        if (!image || !image->isValid())
        {
            spdlog::warn("SGSImageItem::setImage: Invalid image region");
            return;
        }
        
        spdlog::info("SGSImageItem::setImage: {}x{}", image->m_width, image->m_height);

    // Protect access to shared data (m_full_image, m_image_width, m_image_height, m_texture_needs_update)
    {
        QMutexLocker lock(&m_image_mutex);
        m_full_image = image;
        m_image_width = image->m_width;
        m_image_height = image->m_height;
    }

    updateTextureOnMainThread();

    // Emit signal for QML binding
    emit imageSizeChanged();

    // Trigger a repaint to reflect the new image.
    update();
}

    // Updates a specific tile of the displayed image.
    // Merges the tile data into the full image buffer (CPU side) and marks the GPU texture for update.
    void SGSImageItem::updateTile(const std::shared_ptr<Core::Common::ImageRegion>& tile)
    {
        if (!tile || !tile->isValid())
        {
            spdlog::warn("SGSImageItem::updateTile: Invalid tile");
            return;
        }

    // Protect access to shared data (m_fullImage, m_textureNeedsUpdate)
    {
        QMutexLocker lock(&m_image_mutex);
        if (!m_full_image)
        {
            spdlog::warn("SGSImageItem::updateTile: No base image loaded");
            return;
        }

        // Check bounds
        if (tile->m_x < 0 || tile->m_y < 0 ||
            tile->m_x + tile->m_width > m_full_image->m_width ||
            tile->m_y + tile->m_height > m_full_image->m_height) {
            spdlog::warn("SGSImageItem::updateTile: Tile out of bounds");
            return;
        }

        // Iterate through the pixels of the tile.
        for (int y = 0; y < tile->m_height; ++y)
        {
            for (int x = 0; x < tile->m_width; ++x)
            {
                // Iterate through the channels (e.g., RGBA).
                for (int c = 0; c < tile->m_channels; ++c)
                {
                    // Copy the pixel value from the tile to the correct position in the full image.
                    // Use the operator() of ImageRegion assuming (y, x, c) indexing.
                    (*m_full_image)(tile->m_y + y, tile->m_x + x, c) = (*tile)(y, x, c);
                }
            }
        }
    }

    updateTextureOnMainThread();

    spdlog::debug("SGSImageItem::updateTile: Merged tile at ({}, {}) {}x{}",
                  tile->m_x, tile->m_y, tile->m_width, tile->m_height);
    // Trigger a repaint to reflect the updated tile.
    update();
}

// Updates the scene graph node for this item.
// Creates or updates the QSGSimpleTextureNode responsible for rendering.
QSGNode* SGSImageItem::updatePaintNode(QSGNode* node, UpdatePaintNodeData* data)
{
    // Cast the existing node to SGSImageNode, or create a new one if it doesn't exist.
    auto* textureNode = dynamic_cast<QSGSimpleTextureNode*>(node);
    if (!textureNode) {
        textureNode = new QSGSimpleTextureNode();
        spdlog::info("SGSImageItem::updatePaintNode: Created new QSGSimpleTextureNode");
    } else {
        spdlog::info("SGSImageItem::updatePaintNode: Reusing existing QSGSimpleTextureNode");
    }

    // 1. Transfer the new texture created on the main thread to the render thread
    {
        QMutexLocker pending_lock(&m_pending_mutex);
        if (m_pending_texture) {
            // The QSGSimpleTextureNode takes ownership of the texture.
            // The old texture (if any) will be deleted by Qt Quick when the node is destroyed.
            textureNode->setTexture(m_pending_texture);
            m_pending_texture = nullptr; // Clear the pending texture pointer
            spdlog::info("SGSImageItem::updatePaintNode: Set new texture from main thread");
        }
        // If m_pending_texture is nullptr, the node keeps its current texture
    }

    // 2. Configure the node with the current texture (either new or old)
        if (textureNode->texture()) {
        // Apply zoom and pan via the node's rect. This scales and translates the texture on screen.
        // The anchor point for scaling is the top-left corner (0, 0) by default.
        // To center the zoom, you might need a QSGTransformNode or adjust the rect's top-left accordingly.
        float display_width = m_image_width * m_zoom;
        float display_height = m_image_height * m_zoom;
        // Example: center pan relative to item size
        float offset_x = m_pan.x() - (display_width - width()) / 2.0f;
        float offset_y = m_pan.y() - (display_height - height()) / 2.0f;
        // Note: Adjust offset calculation based on desired pan behavior (relative to item or texture)
        textureNode->setRect(offset_x, offset_y, display_width, display_height);
        textureNode->setFiltering(QSGTexture::Linear); // Or Nearest, depending on preference
        spdlog::warn("SGSImageItem::updatePaintNode: configure node success");

        // Note: The pan calculation above is an example. You might want pan to be relative to the item's center
        // or the texture's center, which requires more complex offset logic here or in setPan.
    } else {
        // If no texture is available, clear the node's texture and set its rect to zero.
        textureNode->setTexture(nullptr);
        textureNode->setRect(0, 0, 0, 0);
        spdlog::warn("SGSImageItem::updatePaintNode: No texture available for rendering");
    }

    spdlog::info("SGSImageItem::updatePaintNode: Node updated, texture set: {}", textureNode->texture() ? "yes" : "no");
    // Return the render node to the scene graph.
    return textureNode;
}

// Converts the internal ImageRegion to a QSGTexture on the main thread.
void SGSImageItem::updateTextureOnMainThread()
{
    // This function runs on the **main thread**.
    // m_image_mutex protects m_full_image and m_image_width/height accessed here.
    std::shared_ptr<Core::Common::ImageRegion> current_image;
    int current_width = 0;
    int current_height = 0;
    int current_channels = 0;

    // 1. Read the current image data safely
    {
        QMutexLocker lock(&m_image_mutex);
        if (!m_full_image) {
            spdlog::warn("SGSImageItem::updateTextureOnMainThread: No image data to convert");
            // No need to set m_pending_texture to nullptr here, it's handled by updatePaintNode
            return;
        }
        current_image = m_full_image; // Copy the shared_ptr
        current_width = m_image_width;
        current_height = m_image_height;
        current_channels = m_full_image->m_channels;
    }

    if (!current_image) return;

    spdlog::debug("SGSImageItem::updateTextureOnMainThread: Converting ImageRegion ({}x{}) to QImage",
                  current_width, current_height);

    // 2. Convert ImageRegion (float32) to QImage (uint8)
    QImage::Format qimage_format = (current_channels == 3) ? QImage::Format_RGB888 : QImage::Format_RGBA8888;
    QImage qimage(current_width, current_height, qimage_format);

    for (int y = 0; y < current_height; ++y) {
        for (int x = 0; x < current_width; ++x) {
            size_t baseIdx = (y * current_width + x) * current_channels;
            if (baseIdx + current_channels - 1 < current_image->m_data.size()) {
                float r = std::clamp(current_image->m_data[baseIdx + 0], 0.0f, 1.0f);
                float g = std::clamp(current_image->m_data[baseIdx + 1], 0.0f, 1.0f);
                float b = std::clamp(current_image->m_data[baseIdx + 2], 0.0f, 1.0f);
                float a = (current_channels == 4) ? std::clamp(current_image->m_data[baseIdx + 3], 0.0f, 1.0f) : 1.0f;

                QRgb pixelValueRgb = qRgba(
                    static_cast<uchar>(r * 255),
                    static_cast<uchar>(g * 255),
                    static_cast<uchar>(b * 255),
                    static_cast<uchar>(a * 255)
                    );
                qimage.setPixel(x, y, pixelValueRgb);
            }
        }
    }

    // 3. Create new QSGTexture from QImage on the main thread
    QSGTexture* new_texture = nullptr;
    if (window()) { // Ensure window is valid on the main thread
        spdlog::info("SGSImageItem::updateTextureOnMainThread: Window pointer is valid, attempting to create texture.");
        new_texture = window()->createTextureFromImage(qimage);
        if (new_texture) {
            spdlog::info("SGSImageItem::updateTextureOnMainThread: QSGTexture created successfully from QImage");
        } else {
            spdlog::info("SGSImageItem::updateTextureOnMainThread: Failed to create QSGTexture from QImage");
        }
    } else {
        spdlog::error("SGSImageItem::updateTextureOnMainThread: No window available to create texture");
    }

    // 4. Store the new texture in m_pending_texture for the render thread to pick up
    {
        QMutexLocker pending_lock(&m_pending_mutex);
        // The old m_pending_texture (if any) should ideally be deleted here or managed by Qt Quick.
        // However, deleting it here might be unsafe if the render thread is still using it.
        // Qt Quick's resource management usually handles this correctly when the texture is set on a node.
        // For now, we just overwrite the pointer. The old texture will be deleted by Qt Quick when appropriate.
        // A more robust solution might involve a double-buffering mechanism or using QQuickWindow::scheduleRenderJob.
        delete m_pending_texture; // Delete the old pending texture if it exists
        m_pending_texture = new_texture; // Assign the new texture
    }
}

// Sets the zoom level.
void SGSImageItem::setZoom(float zoom)
{
    if (!qFuzzyCompare(m_zoom, zoom) && zoom > 0.0f)
    { // Verify the positive value zoom
        m_zoom = zoom;
        emit zoomChanged(m_zoom); // Emit signal for QML binding
        update(); // Trigger repaint
    }
}

// Sets the pan offset.
void SGSImageItem::setPan(const QPointF& pan)
{
    if (m_pan != pan)
    {
        m_pan = pan;
        emit panChanged(m_pan); // Emit signal for QML binding
        update(); // Trigger repaint
    }
}

} // namespace CaptureMoment::UI::Rendering
