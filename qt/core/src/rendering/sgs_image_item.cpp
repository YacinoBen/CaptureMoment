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
    : QQuickItem(parent) // Appel du constructeur de QQuickItem
{
    // Indicate to Qt Quick that this item has custom content rendered via the scene graph.
    setFlag(QQuickItem::ItemHasContents, true);
    spdlog::info("SGSImageItem: Created");
}

// Destructor: Cleans up resources.
SGSImageItem::~SGSImageItem() {
    // The deletion of textures managed by QSGNodes is handled by Qt Quick
    // when the nodes are destroyed.
    spdlog::debug("SGSImageItem: Destroyed");
}

// Sets the full image to be displayed.
// Updates the internal image data and marks the internal state for update.
void SGSImageItem::setImage(const std::shared_ptr<Core::Common::ImageRegion>& image)
{
    if (!image || !image->isValid())
    {
        spdlog::warn("SGSImageItem::setImage: Invalid image region");
        return;
    }

    spdlog::info("SGSImageItem::setImage: {}x{}", image->m_width, image->m_height);

    // Protect access to shared data (m_full_image, m_image_width, m_image_height, m_image_dirty)
    // This lock is held while updating the core image data members.
    {
        QMutexLocker lock(&m_image_mutex);
        m_full_image = image; // Update the core image data pointer
        m_image_width = image->m_width; // Update the core image width
        m_image_height = image->m_height; // Update the core image height
        m_image_dirty = true; // Flag to indicate the image data has changed
    }

    // Emit signal for QML binding (BaseImageItem manages the signal emission)
    emit imageSizeChanged();

    // Trigger a repaint to reflect the new image.
    update();
}

// Updates a specific tile of the displayed image.
// Merges the tile data into the full image buffer (CPU side) and marks the internal state for update.
void SGSImageItem::updateTile(const std::shared_ptr<Core::Common::ImageRegion>& tile)
{
    if (!tile || !tile->isValid())
    {
        spdlog::warn("SGSImageItem::updateTile: Invalid tile");
        return;
    }

    // Protect access to shared data (m_full_image, m_image_dirty)
    // This lock is held while updating the core image data.
    {
        QMutexLocker lock(&m_image_mutex);
        if (!m_full_image)
        {
            spdlog::warn("SGSImageItem::updateTile: No base image loaded");
            return;
        }

        // Check bounds to ensure the tile fits within the main image region
        if (tile->m_x < 0 || tile->m_y < 0 ||
            tile->m_x + tile->m_width > m_full_image->m_width ||
            tile->m_y + tile->m_height > m_full_image->m_height) {
            spdlog::warn("SGSImageItem::updateTile: Tile out of bounds");
            return;
        }

        // Iterate through the pixels of the tile and copy data into the main image
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
        m_image_dirty = true; // Flag to indicate the image data has changed due to tile update
    }

    spdlog::info("SGSImageItem::updateTile: Merged tile at ({}, {}) {}x{}",
                 tile->m_x, tile->m_y, tile->m_width, tile->m_height);
    // Trigger a repaint to reflect the updated tile.
    update();
}

// Updates the scene graph node for this item.
// Creates or updates the QSGSimpleTextureNode responsible for rendering.
QSGNode* SGSImageItem::updatePaintNode(QSGNode* node, UpdatePaintNodeData* data)
{
    // Cast the existing node to QSGSimpleTextureNode, or create a new one if it doesn't exist.
    auto* textureNode = dynamic_cast<QSGSimpleTextureNode*>(node);
    if (!textureNode) {
        textureNode = new QSGSimpleTextureNode();
        spdlog::info("SGSImageItem::updatePaintNode: Created new QSGSimpleTextureNode");
    } else {
        spdlog::info("SGSImageItem::updatePaintNode: Reusing existing QSGSimpleTextureNode");
    }

    // This function runs on the render thread.
    // Check if the image data has changed and needs to be converted to a texture.
    {
        QMutexLocker lock(&m_image_mutex); // Lock to access m_image_dirty and m_full_image safely
        if (m_image_dirty && m_full_image) // Only proceed if data is dirty and available
        {
            spdlog::debug("SGSImageItem::updatePaintNode: Converting ImageRegion to QImage and QSGTexture");

            // 1. Conversion: ImageRegion (float32) -> QImage (uint8) on the render thread
            int w = m_image_width; // Use the core image width
            int h = m_image_height; // Use the core image height
            int ch = m_full_image->m_channels; // Use the core image channel count
            QImage::Format qimage_format = (ch == 3) ? QImage::Format_RGB888 : QImage::Format_RGBA8888;
            QImage qimage(w, h, qimage_format);

            // Iterate through the pixels of the ImageRegion and convert to uint8
            for (int y = 0; y < h; ++y)
            {
                uchar* line = qimage.scanLine(y); // Get pointer to the scanline for direct pixel access
                for (int x = 0; x < w; ++x)
                {
                    size_t idx = (y * w + x) * ch; // Calculate the index in the flat data array
                    // Clamp the float value to [0.0, 1.0] and scale to [0, 255]
                    line[x * ch + 0] = static_cast<uchar>(std::clamp(m_full_image->m_data[idx + 0], 0.0f, 1.0f) * 255);
                    line[x * ch + 1] = static_cast<uchar>(std::clamp(m_full_image->m_data[idx + 1], 0.0f, 1.0f) * 255);
                    line[x * ch + 2] = static_cast<uchar>(std::clamp(m_full_image->m_data[idx + 2], 0.0f, 1.0f) * 255);
                    if (ch == 4) { // Handle alpha channel if present
                        line[x * ch + 3] = static_cast<uchar>(std::clamp(m_full_image->m_data[idx + 3], 0.0f, 1.0f) * 255);
                    }
                }
            }

            // 2. Creation of the QSGTexture on the render thread using the converted QImage
            QSGTexture* old_texture = textureNode->texture(); // Store the old texture pointer

            // Create the new texture using the window's RHI context (safe on render thread)
            QSGTexture* new_texture = window()->createTextureFromImage(qimage);

            if (new_texture) // Check if texture creation was successful
            {
                textureNode->setTexture(new_texture); // Assign the new texture to the node
                textureNode->setOwnsTexture(true);    // Let the node manage the texture's lifetime

                // The old texture will be deleted automatically by Qt Quick when the node is destroyed
                // or when a new texture is set (if setOwnsTexture(true) was called before).
                // If setOwnsTexture was not called, old_texture would need manual deletion here.
                // Since we call it here, the old texture is managed by the node.
            } else {
                spdlog::error("SGSImageItem::updatePaintNode: Failed to create QSGTexture from QImage");
            }

            m_image_dirty = false; // Reset the dirty flag after processing
        }
    }

    // Configure the texture node's rendering properties (rect, filtering)
    // This happens regardless of whether the texture was updated or not.
    if (textureNode->texture()) // Only configure if a texture exists
    {
        // Calculate the display size based on zoom
        float display_width = m_image_width * m_zoom; // Use core image width and zoom level
        float display_height = m_image_height * m_zoom; // Use core image height and zoom level

        // Calculate the position (x, y) to apply pan offset relative to the item's center
        // This centers the image within the item's bounds and applies pan translation.
        float x_pos = (width() - display_width) / 2.0f + m_pan.x(); // Use item width and pan x
        float y_pos = (height() - display_height) / 2.0f + m_pan.y(); // Use item height and pan y

        // Set the rectangle where the texture will be drawn
        textureNode->setRect(x_pos, y_pos, display_width, display_height);

        // Set the texture filtering (e.g., linear for smooth scaling)
        textureNode->setFiltering(QSGTexture::Linear);
    } else {
        // If no texture is available, clear the node's rect
        textureNode->setRect(0, 0, 0, 0);
        spdlog::warn("SGSImageItem::updatePaintNode: No texture available for rendering");
    }

    spdlog::info("SGSImageItem::updatePaintNode: Node updated, texture set");
    // Return the render node to the scene graph.
    return textureNode;
}

// Sets the zoom level.
void SGSImageItem::setZoom(float zoom)
{
    // Check if the zoom value is different and positive
    if (!qFuzzyCompare(m_zoom, zoom) && zoom > 0.0f)
    {
        m_zoom = zoom; // Update the core zoom value (from BaseImageItem)
        emit zoomChanged(m_zoom); // Emit signal for QML binding
        update(); // Trigger repaint to reflect the new zoom level
    }
}

// Sets the pan offset.
void SGSImageItem::setPan(const QPointF& pan)
{
    // Check if the pan value is different
    if (m_pan != pan)
    {
        m_pan = pan; // Update the core pan value (from BaseImageItem)
        emit panChanged(m_pan); // Emit signal for QML binding
        update(); // Trigger repaint to reflect the new pan offset
    }
}

} // namespace CaptureMoment::UI::Rendering
