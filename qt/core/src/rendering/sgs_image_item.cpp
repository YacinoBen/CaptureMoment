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
#include <algorithm> // Pour std::clamp
#include <cstring>   // Pour std::memcpy si nécessaire

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
        delete m_cached_texture; // Nettoyer la texture si elle existe
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
        emit imageSizeChanged();

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
            textureNode->setRect(0, 0, m_image_width * m_zoom, m_image_height * m_zoom); // Apply zoom/pan via node rect or transform
            textureNode->setFiltering(QSGTexture::Linear); // Or Nearest, depending on preference

            // Apply pan/zoom via a transform node if needed, instead of just scaling the rect
            // This gives more control over the anchor point of zoom/pan.
            // For now, let's assume zoom/pan are handled by adjusting the rect or by a parent transform node.
            // QSGTransformNode could be used if precise control over the transformation matrix is needed.
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
            delete m_cached_texture; // Clean up old texture if present
            m_cached_texture = nullptr;
            return;
        }

        spdlog::debug("SGSImageItem::updateCachedTexture: Converting Core::Common::ImageRegion ({}x{}) to QImage",
                     m_full_image->m_width, m_full_image->m_height);

        // --- Convert Core::Common::ImageRegion (float32) to QImage (uint8) ---
        QImage qimage(m_full_image->m_width, m_full_image->m_height, QImage::Format_RGBA8888);

        for (int y = 0; y < m_full_image->m_height; ++y) {
            for (int x = 0; x < m_full_image->m_width; ++x) {
                QRgb pixelValue = 0;
                size_t baseIdx = (y * m_full_image->m_width + x) * 4; // Supposons 4 canaux (RGBA)
                if (baseIdx + 3 < m_full_image->m_data.size()) {
                    float r = std::clamp(m_full_image->m_data[baseIdx + 0], 0.0f, 1.0f);
                    float g = std::clamp(m_full_image->m_data[baseIdx + 1], 0.0f, 1.0f);
                    float b = std::clamp(m_full_image->m_data[baseIdx + 2], 0.0f, 1.0f);
                    float a = std::clamp(m_full_image->m_data[baseIdx + 3], 0.0f, 1.0f); // Supposons un canal alpha

                    pixelValue = qRgba(
                        static_cast<uchar>(r * 255),
                        static_cast<uchar>(g * 255),
                        static_cast<uchar>(b * 255),
                        static_cast<uchar>(a * 255)
                    );
                }
                qimage.setPixel(x, y, pixelValue);
            }
        }

        // --- Clean up old texture ---
        delete m_cached_texture;
        m_cached_texture = nullptr;

        // --- Create new QSGTexture from QImage ---
        // This method creates the texture on the GPU via the window's RHI context.
        if (window()) {
            m_cached_texture = window()->createTextureFromImage(qimage);
            if (m_cached_texture) {
                spdlog::debug("SGSImageItem::updateCachedTexture: QSGTexture created successfully from QImage");
            } else {
                spdlog::error("SGSImageItem::updateCachedTexture: Failed to create QSGTexture from QImage");
            }
        } else {
            spdlog::error("SGSImageItem::updateCachedTexture: No window available to create texture");
        }
    }

} // namespace CaptureMoment::UI::Rendering
