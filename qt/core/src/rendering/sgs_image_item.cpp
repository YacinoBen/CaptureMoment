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
    : QQuickItem(parent)
{
    // Indicate to Qt Quick that this item has custom content rendered via the scene graph.
    setFlag(QQuickItem::ItemHasContents, true);
    setSize(QSizeF(800, 600)); // Taille par défaut
    spdlog::info("SGSImageItem: Created");
}

// Destructor: Cleans up resources.
SGSImageItem::~SGSImageItem() {
    // La suppression de m_cached_texture est faite dans updateCachedTexture
    // ou dans le destructeur du QSGNode si elle est attachée au nœud.
    // On s'assure ici que le thread de rendu n'utilise plus la texture.
    // Cela se fait souvent automatiquement via les mécanismes de Qt Quick.
    // delete m_cached_texture; // <-- DANGEREUX ici si la texture est attachée au nœud du thread de rendu
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
        // Signal that the GPU texture needs to be updated from the new image data.
        m_texture_needs_update = true;
    }

    // Emit signal for QML binding
    emit imageDimensionsChanged();

    // Trigger a repaint to reflect the new image.
    update();
}

// Sets the zoom level.
void SGSImageItem::setZoom(float zoom)
{
    if (!qFuzzyCompare(m_zoom, zoom)) {
        m_zoom = zoom;
        emit zoomChanged(m_zoom); // Emit signal for QML binding
        update(); // Trigger repaint
    }
}

// Sets the pan offset.
void SGSImageItem::setPan(const QPointF& pan)
{
    if (m_pan != pan) {
        m_pan = pan;
        emit panChanged(m_pan); // Emit signal for QML binding
        update(); // Trigger repaint
    }
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
                        // This assumes Core::Common::ImageRegion supports operator() for (x, y, c) access.
                        // Adjust the indexing if Core::Common::ImageRegion uses (y, x, c) or another convention.
                        // (*m_full_image)(tile->m_y + y, tile->m_x + x, c) = (*tile)(y, x, c);
                        // Ou, si Core::Common::ImageRegion utilise (x, y, c) :
                        (*m_full_image)(tile->m_x + x, tile->m_y + y, c) = (*tile)(x, y, c);
                    }
                }
            }
             // Signal that the GPU texture needs to be updated due to the tile change.
            m_texture_needs_update = true;
        }
        
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

        // Check if texture needs update (thread-safe check)
        if (m_texture_needs_update) {
            updateCachedTexture(); // Convert Core::Common::ImageRegion -> QImage -> QSGTexture
            m_texture_needs_update = false; // Reset flag
        }

    // Set the texture on the node if we have one cached
    if (m_cached_texture) {
        textureNode->setTexture(m_cached_texture);
        // Apply zoom and pan via the node's rect. This scales and translates the texture on screen.
        // The anchor point for scaling is the top-left corner (0, 0) by default.
        // To center the zoom, you might need a QSGTransformNode or adjust the rect's top-left accordingly.
        float display_width = m_image_width * m_zoom;
        float display_height = m_image_height * m_zoom;
        float offset_x = m_pan.x() - (display_width - width()) / 2.0f; // Example: center pan relative to item size
        float offset_y = m_pan.y() - (display_height - height()) / 2.0f; // Example: center pan relative to item size
        // Note: Adjust offset calculation based on desired pan behavior (relative to item or texture)
        textureNode->setRect(offset_x, offset_y, display_width, display_height);
        textureNode->setFiltering(QSGTexture::Linear); // Or Nearest, depending on preference

        // Note: The pan calculation above is an example. You might want pan to be relative to the item's center
        // or the texture's center, which requires more complex offset logic here or in setPan.
    } else {
        // If no texture is available, clear the node's texture and set its rect to zero.
        textureNode->setTexture(nullptr);
        textureNode->setRect(0, 0, 0, 0);
        spdlog::warn("SGSImageItem::updatePaintNode: No cached texture available");
    }

    spdlog::trace("SGSImageItem::updatePaintNode: Node updated, texture set: {}", m_cached_texture ? "yes" : "no");
    // Return the render node to the scene graph.
    return textureNode;
}

    // Converts the internal Core::Common::ImageRegion to a QSGTexture.
    void SGSImageItem::updateCachedTexture() {
        QMutexLocker lock(&m_image_mutex);

    if (!m_full_image) {
        spdlog::warn("SGSImageItem::updateCachedTexture: No image data");
        // The old texture will be deleted by Qt Quick when the node is destroyed
        // or when a new texture is set. We just set the member to nullptr.
        // delete m_cached_texture; // <-- DANGEREUX sur le thread de rendu
        m_cached_texture = nullptr;
        return;
    }

        spdlog::debug("SGSImageItem::updateCachedTexture: Converting Core::Common::ImageRegion ({}x{}) to QImage",
                     m_full_image->m_width, m_full_image->m_height);

        // --- Convert Core::Common::ImageRegion (float32) to QImage (uint8) ---
        QImage qimage(m_full_image->m_width, m_full_image->m_height, QImage::Format_RGBA8888);

    for (int y = 0; y < m_full_image->m_height; ++y) {
        for (int x = 0; x < m_full_image->m_width; ++x) {
            size_t baseIdx = (y * m_full_image->m_width + x) * m_full_image->m_channels;
            if (baseIdx + m_full_image->m_channels - 1 < m_full_image->m_data.size()) {
                float r = std::clamp(m_full_image->m_data[baseIdx + 0], 0.0f, 1.0f);
                float g = std::clamp(m_full_image->m_data[baseIdx + 1], 0.0f, 1.0f);
                float b = std::clamp(m_full_image->m_data[baseIdx + 2], 0.0f, 1.0f);
                float a = (m_full_image->m_channels == 4) ? std::clamp(m_full_image->m_data[baseIdx + 3], 0.0f, 1.0f) : 1.0f;

                if (m_full_image->m_channels == 3) {
                    QRgb pixelValue = qRgb(
                        static_cast<uchar>(r * 255),
                        static_cast<uchar>(g * 255),
                        static_cast<uchar>(b * 255)
                        );
                    qimage.setPixel(x, y, pixelValue);
                } else { // 4 channels (RGBA)
                    QRgba64 pixelValue = qRgba64( // Utilisation de QRgba64 pour plus de précision
                        static_cast<ushort>(r * 65535),
                        static_cast<ushort>(g * 65535),
                        static_cast<ushort>(b * 65535),
                        static_cast<ushort>(a * 65535)
                        );
                    // Ou pour QRgb (moins précis mais plus courant)
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
    }

    // --- Clean up old texture ---
    // The old texture should be deleted by Qt Quick when the node is destroyed or replaced.
    // We simply create a new one and let the scene graph manage the old one.
    // delete m_cached_texture; // <-- DANGEREUX sur le thread de rendu
    // m_cached_texture = nullptr; // Not necessary before creating a new one

    // --- Create new QSGTexture from QImage ---
    // This method creates the texture on the GPU via the window's RHI context.
    // It's safe to call on the rendering thread if `window()` is accessible here.
    // However, `window()` might not be available in `updatePaintNode` context reliably.
    // A safer way is to use `window()->createTextureFromImage(qimage)` inside `updatePaintNode`
    // after checking `window()`, but we need to manage the lifetime correctly.
    // For simplicity here, we assume `window()` is valid in the context where updateCachedTexture is called
    // (which should be from updatePaintNode on the render thread).

    // Ensure we have a valid window pointer (this should be true in updatePaintNode)
    if (window())
    {
        // The texture will be managed by the QSGNode. Qt Quick will handle its deletion.
        QSGTexture* new_texture = window()->createTextureFromImage(qimage);
        if (new_texture) {
            m_cached_texture = new_texture; // Assign the new texture
            spdlog::debug("SGSImageItem::updateCachedTexture: QSGTexture created successfully from QImage");
        } else {
            spdlog::error("SGSImageItem::updateCachedTexture: Failed to create QSGTexture from QImage");
            m_cached_texture = nullptr; // Ensure member is null on failure
        }
    } else {
        spdlog::error("SGSImageItem::updateCachedTexture: No window available to create texture");
        m_cached_texture = nullptr; // Ensure member is null if no window
    }
}

// Gets the width of the image.
int SGSImageItem::imageWidth() const
{
    QMutexLocker lock(&m_image_mutex);
    return m_image_width;
}

// Gets the height of the image.
int SGSImageItem::imageHeight() const
{
    QMutexLocker lock(&m_image_mutex);
    return m_image_height;
}

} // namespace CaptureMoment::UI::Rendering
