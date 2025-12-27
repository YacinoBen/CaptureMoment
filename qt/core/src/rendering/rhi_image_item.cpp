/**
 * @file rhi_image_item.cpp
 * @brief Modern RHI implementation (Qt 6, multi-API)
 * @author CaptureMoment Team
 * @date 2025
 */
#include <spdlog/spdlog.h>
#include <QMutexLocker>
#include "rendering/rhi_image_item.h"
#include "rendering/rhi_image_node.h" 
namespace CaptureMoment::UI::Rendering {
// --- RHIImageItem Implementation ---
RHIImageItem::RHIImageItem(QQuickItem* parent)
        : BaseImageItem(parent)
{
        // Indicate to Qt Quick that this item has custom content rendered via the scene graph.
        setFlag(QQuickItem::ItemHasContents, true);
        setSize(QSizeF(800, 600));
        spdlog::info("RHIImageItem: Created");
}
// Destructor: Cleans up resources.
RHIImageItem::~RHIImageItem() {
    spdlog::debug("RHIImageItem: Destroyed");
}
// Sets the full image to be displayed.
// Updates the internal image data and marks the GPU texture for update.
void RHIImageItem::setImage(const std::shared_ptr<Core::Common::ImageRegion>& image)
{
    if (!image || !image->isValid())
    {
        spdlog::warn("RHIImageItem::setImage: Invalid image region");
        return;
    }
    spdlog::info("RHIImageItem::setImage ((QMutexLocker) try enter lock ): {}x{}", m_image_width, m_image_height);
    // Protect access to shared data (m_full_image, m_image_width, m_image_height, m_texture_needs_update)
    {
        QMutexLocker lock(&m_image_mutex);
        spdlog::info("RHIImageItem::setImage ((QMutexLocker) in the lock ): {}x{}", m_image_width, m_image_height);
        m_full_image = image;
        m_image_width = image->m_width;
        m_image_height = image->m_height;
        // Signal that the GPU texture needs to be updated from the new image data.
        m_texture_needs_update = true;
    }
    spdlog::info("RHIImageItem::setImage: {}x{}", m_image_width, m_image_height);
    // Trigger a repaint to reflect the new image.
    update();
}
void RHIImageItem::setZoom(float zoom) 
{
    BaseImageItem::setZoom(zoom);
    update(); // Trigger repaint
}
void RHIImageItem::setPan(const QPointF& pan)
{
    BaseImageItem::setPan(pan);
    update(); // Trigger repaint
}

// Updates a specific tile of the displayed image.
// Merges the tile data into the full image buffer (CPU side) and marks the GPU texture for update.
void RHIImageItem::updateTile(const std::shared_ptr<Core::Common::ImageRegion>& tile)
{
    if (!tile || !tile->isValid())
    {
        spdlog::warn("RHIImageItem::updateTile: Invalid tile");
        return;
    }
    // Protect access to shared data (m_fullImage, m_textureNeedsUpdate)
    {
        QMutexLocker lock(&m_image_mutex);
        if (!m_full_image) 
        {
            spdlog::warn("RHIImageItem::updateTile: No base image loaded");
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
                    // This assumes ImageRegion supports operator() for (x, y, c) access.
                    // Adjust the indexing if ImageRegion uses (y, x, c) or another convention.
                    (*m_full_image)(tile->m_y + y, tile->m_x + x, c) = (*tile)(y, x, c);
                }
            }
        }
         // Signal that the GPU texture needs to be updated due to the tile change.
        m_texture_needs_update = true;
    }
    spdlog::debug("RHIImageItem::updateTile: Merged tile at ({}, {})", tile->m_x, tile->m_y);
    // Trigger a repaint to reflect the updated tile.
    update();
}
// Updates the scene graph node for this item.
// Creates or updates the RHIImageNode responsible for rendering via QRhi.
QSGNode* RHIImageItem::updatePaintNode(QSGNode* node, UpdatePaintNodeData* data)
{
    // Cast the existing node to RHIImageNode, or create a new one if it doesn't exist.
    auto* m_render_node = dynamic_cast<RHIImageNode*>(node);
    if (!m_render_node) {
        m_render_node = new RHIImageNode(this);
        spdlog::info("RHIImageItem::updatePaintNode (QMutexLocker) : m_render_node create new : {}",
                m_render_node ? "valid" : "null");
    } else {
    spdlog::info("RHIImageItem::updatePaintNode (QMutexLocker) : m_render_node existing : {}",
                m_render_node ? "valid" : "null");
    }
    // Synchronize state (like texture update flag) from the GUI thread to the render node.
    spdlog::info("RHIImageItem::updatePaintNode: calling m_render_node->synchronize()");
    m_render_node->synchronize();
    // Mark the material as dirty to ensure the render node is redrawn with the latest state.
    m_render_node->markDirty(QSGNode::DirtyMaterial);
        // Return the render node to the scene graph.
    return m_render_node;
}
} // namespace CaptureMoment::Qt::Rendering
