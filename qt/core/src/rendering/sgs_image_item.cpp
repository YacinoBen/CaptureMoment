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
#include <QColorSpace>

namespace CaptureMoment::UI::Rendering {

// Constructor: Initializes the item and sets the flag for custom content.
SGSImageItem::SGSImageItem(QQuickItem* parent)
    : QQuickItem(parent)
{
    // Indicate to Qt Quick that this item has custom content rendered via the scene graph.
    setFlag(QQuickItem::ItemHasContents, true);
    spdlog::debug("[SGSImageItem::SGSImageItem]: Created");
}

// Destructor: Cleans up resources.
SGSImageItem::~SGSImageItem() {
    // The deletion of textures managed by QSGNodes is handled by Qt Quick
    // when the nodes are destroyed.
    spdlog::debug("[SGSImageItem::~SGSImageItem()]: Destroyed");
}

// Sets the full image to be displayed.
// Updates the internal image data and marks the internal state for update.
void SGSImageItem::setImage(std::unique_ptr<Core::Common::ImageRegion> image)
{
    if (!image || !image->isValid())
    {
        spdlog::warn("[SGSImageItem::setImage]: Invalid image region");
        return;
    }

    spdlog::info("[SGSImageItem::setImage]: {}x{}", image->width(), image->height());

    // This lock is held while updating the core image data members.
    {
        QMutexLocker lock(&m_image_mutex);
        m_full_image = std::move(image);
        m_image_dirty = true;
    }

    // Emit signal for QML binding (BaseImageItem manages the signal emission)
    emit imageSizeChanged();

    // Trigger a repaint to reflect the new image.
    QMetaObject::invokeMethod(this, &QQuickItem::update, Qt::QueuedConnection);
}

// Updates a specific tile of the displayed image.
// Merges the tile data into the full image buffer (CPU side) and marks the internal state for update.
void SGSImageItem::updateTile(std::unique_ptr<Core::Common::ImageRegion> tile)
{
    if (!tile || !tile->isValid()) {
        spdlog::warn("[SGSImageItem::updateTile]: Invalid tile");
        return;
    }

    {
        QMutexLocker lock(&m_image_mutex);

        if (!m_full_image) {
            spdlog::warn("[SGSImageItem::updateTile]: No base image");
            return;
        }

        // Full replacement if same dimensions (most common case)
        if (tile->width() == m_full_image->width() &&
            tile->height() == m_full_image->height())
        {
            // Copy data instead of move to avoid destructor issues
            const size_t data_size { tile->width() * tile->height() * tile->channels() };
            std::copy(tile->getBuffer().data(),
                      tile->getBuffer().data() + data_size,
                      m_full_image->getBuffer().data());
            m_image_dirty = true;
            spdlog::info("[SGSImageItem::updateTile]: Full copy {}x{}", tile->width(), tile->height());
        } else
        {
            // Partial tile update
            if (tile->x() < 0 || tile->y() < 0 ||
                tile->x() + tile->width() > m_full_image->width() ||
                tile->y() + tile->height() > m_full_image->height()) {
                spdlog::warn("[SGSImageItem::updateTile]: Tile out of bounds");
                return;
            }

            // Copy the tile data row by row
            const size_t row_size { tile->width() * tile->channels() };
            const int full_w { static_cast<int>(m_full_image->width()) };
            const int tile_x { tile->x() };
            const int tile_y { tile->y() } ;
            const int tile_h { static_cast<int>(tile->height()) };

            for (size_t y = 0; y < tile_h; ++y)
            {
                const float* src = tile->getBuffer().data() + y * row_size;
                // FIXED: (tile_y + y) not (tile_x + y)
                float* dst = m_full_image->getBuffer().data() +
                             ((tile_y + y) * full_w + tile_x) * m_full_image->channels();
                std::copy(src, src + row_size, dst);
            }
            m_image_dirty = true;
            spdlog::debug("[SGSImageItem::updateTile]: Partial copy at ({}, {})", tile_x, tile_y);
        }
    }

    spdlog::debug("[SGSImageItem::updateTile]: Merged tile at ({}, {})", tile->x(), tile->y());
    QMetaObject::invokeMethod(this, &QQuickItem::update, Qt::QueuedConnection);
}

// Updates the scene graph node for this item.
// Creates or updates the QSGSimpleTextureNode responsible for rendering.
QSGNode* SGSImageItem::updatePaintNode(QSGNode* node, UpdatePaintNodeData* data)
{
    Q_UNUSED(data);

    if (!isImageValid()) {
        return node;
    }

    auto* texture_node { static_cast<QSGSimpleTextureNode*>(node) };

    int img_w { 0 };
    int img_h { 0 };
    bool needs_texture_update { false };
    QImage new_qimage;

    {
        QMutexLocker lock(&m_image_mutex);

        if (m_full_image && m_full_image->isValid())
        {
            img_w = static_cast<int>(m_full_image->width());
            img_h = static_cast<int>(m_full_image->height());

            if (m_image_dirty)
            {
                needs_texture_update = true;

                QImage linear_img(
                    reinterpret_cast<const uchar*>(m_full_image->getBuffer().data()),
                    img_w, img_h,
                    img_w * 4 * static_cast<int>(sizeof(float)),
                    QImage::Format_RGBA32FPx4
                    );

                const QColorSpace linear_cs { QColorSpace::SRgbLinear };
                linear_img.setColorSpace(linear_cs);

                new_qimage = linear_img.convertedToColorSpace(QColorSpace::SRgb)
                                 .convertToFormat(QImage::Format_RGBA8888);

                m_image_dirty = false;
            }
        }
    }

    QQuickWindow* win = window();
    if (!win) {
        spdlog::warn("[SGSImageItem::updatePaintNode]: No window attached");
        return node;
    }

    if (!texture_node) {
        texture_node = new QSGSimpleTextureNode();
    }

    if (needs_texture_update && !new_qimage.isNull())
    {
        QSGTexture* new_texture { win->createTextureFromImage(
            new_qimage,
            QQuickWindow::TextureHasAlphaChannel
            ) };

        if (new_texture) {
            texture_node->setTexture(new_texture);
            texture_node->setOwnsTexture(true);
        } else {
            spdlog::warn("[SGSImageItem::updatePaintNode]: Failed to create texture");
        }
    }

    if (texture_node->texture())
    {
        float display_w, display_h;
        if (img_w > 0 && img_h > 0) {
            display_w = img_w * m_zoom;
            display_h = img_h * m_zoom;
        } else {
            display_w = imageWidth() * m_zoom;
            display_h = imageHeight() * m_zoom;
        }

        const float x_pos = (width() - display_w) / 2.0f + m_pan.x();
        const float y_pos = (height() - display_h) / 2.0f + m_pan.y();

        texture_node->setRect(x_pos, y_pos, display_w, display_h);
        texture_node->setFiltering(QSGTexture::Linear);
    }

    return texture_node;
}

void SGSImageItem::onZoomChanged(float new_zoom)
{
    emit zoomChanged(new_zoom);
    update();
}

void SGSImageItem::onPanChanged(const QPointF& new_pan)
{
    emit panChanged(new_pan);
    update();
}

void SGSImageItem::onImageChanged()
{
    emit imageSizeChanged();
    update();
}

} // namespace CaptureMoment::UI::Rendering
