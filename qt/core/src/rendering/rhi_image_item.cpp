/**
 * @file rhi_image_item.cpp
 * @brief Modern RHI implementation (Qt 6, multi-API) using QQuickRhiItem
 * @author CaptureMoment Team
 * @date 2025
 */

#include "rendering/rhi_image_item.h"
#include "rendering/rhi_image_item_renderer.h" // Include the new renderer class
#include <spdlog/spdlog.h>
#include <QMutexLocker>

namespace CaptureMoment::UI::Rendering {

// Constructor: Initializes the item.
RHIImageItem::RHIImageItem(QQuickItem* parent)
    : QQuickRhiItem(parent) // Appel du constructeur de QQuickRhiItem
{
    // QQuickRhiItem sets ItemHasContents automatically
    setSize(QSizeF(800, 600)); // Default size
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
    spdlog::info("RHIImageItem::setImage: CALLED with image {}x{}",
                 image ? image->m_width : -1, image ? image->m_height : -1);

    if (!image || !image->isValid())
    {
        spdlog::warn("RHIImageItem::setImage: Invalid image region");
        return;
    }

    spdlog::info("RHIImageItem::setImage: {}x{}", image->m_width, image->m_height);

    // Protect access to shared data (m_full_image, m_image_width, m_image_height, m_texture_needs_update)
    {
        QMutexLocker lock(&m_image_mutex);
        m_full_image = image;
        m_image_width = image->m_width;
        m_image_height = image->m_height;
        // Signal that the GPU texture needs to be updated from the new image data.
        m_texture_needs_update = true;
        spdlog::info("RHIImageItem::setImage: Texture update flag set to true");
    }

    // Trigger a repaint to reflect the new image.
    spdlog::info("RHIImageItem::setImage: Calling update()");
    update();
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

    // Protect access to shared data (m_full_image, m_texture_needs_update)
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
                    // This assumes ImageRegion supports operator() for (y, x, c) access.
                    // Adjust the indexing if ImageRegion uses (x, y, c) or another convention.
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

bool RHIImageItem::textureNeedsUpdate() const
{
    QMutexLocker lock(&m_image_mutex);
    return m_texture_needs_update;
}

void RHIImageItem::setTextureNeedsUpdate(bool update)
{
    QMutexLocker lock(&m_image_mutex);
    m_texture_needs_update = update;
}

// Sets the zoom level.
void RHIImageItem::setZoom(float zoom)
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
void RHIImageItem::setPan(const QPointF& pan)
{
    // Check if the pan value is different
    if (m_pan != pan)
    {
        m_pan = pan; // Update the core pan value (from BaseImageItem)
        emit panChanged(m_pan); // Emit signal for QML binding
        update(); // Trigger repaint to reflect the new pan offset
    }
}

// Creates the renderer instance responsible for RHI rendering.
QQuickRhiItemRenderer *RHIImageItem::createRenderer()
{
    // Create and return a new instance of the custom renderer.
    return new RHIImageItemRenderer(this); // Pass 'this' to the renderer if needed
}

} // namespace CaptureMoment::UI::Rendering
