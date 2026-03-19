/**
 * @file rhi_image_item.cpp
 * @brief Modern RHI implementation (Qt 6, multi-API) using QQuickRhiItem
 * @author CaptureMoment Team
 * @date 2025
 */

#include "rendering/rhi_image_item.h"
#include "rendering/rhi_image_item_renderer.h"
#include <spdlog/spdlog.h>
#include <QMutexLocker>

namespace CaptureMoment::UI::Rendering {

RHIImageItem::RHIImageItem(QQuickItem* parent)
    : QQuickRhiItem(parent)
{
    spdlog::debug("[RHIImageItem::RHIImageItem]: Created");
}

RHIImageItem::~RHIImageItem()
{
    spdlog::debug("[RHIImageItem::~RHIImageItem]: Destroyed");
}

void RHIImageItem::setImage(std::unique_ptr<Core::Common::ImageRegion> image)
{
    if (!image || !image->isValid()) {
        spdlog::warn("[RHIImageItem::setImage]: Invalid image");
        return;
    }

    spdlog::info("[RHIImageItem::setImage]: {}x{}", image->width(), image->height());

    {
        QMutexLocker lock(&m_image_mutex);
        m_full_image = std::move(image);
        m_texture_needs_update = true;
    }

    onImageChanged();
}

void RHIImageItem::updateTile(std::unique_ptr<Core::Common::ImageRegion> tile)
{
    if (!tile || !tile->isValid()) {
        spdlog::warn("[RHIImageItem::updateTile]: Invalid tile");
        return;
    }

    {
        QMutexLocker lock(&m_image_mutex);

        if (!m_full_image) {
            spdlog::warn("[RHIImageItem::updateTile]: No base image");
            return;
        }

        // Full replacement if same dimensions
        if (tile->width() == m_full_image->width() &&
            tile->height() == m_full_image->height()) {
            m_full_image = std::move(tile);
            spdlog::debug("[RHIImageItem::updateTile]: Full replacement");
        } else {
            // Partial copy by row (optimized)
            const size_t row_size = tile->width() * tile->channels();
            for (int y = 0; y < tile->height(); ++y) {
                const float* src = tile->getBuffer().data() + y * row_size;
                float* dst = m_full_image->getBuffer().data() +
                    ((tile->y() + y) * m_full_image->width() + tile->x()) * m_full_image->channels();
                std::copy(src, src + row_size, dst);
            }
            spdlog::debug("[RHIImageItem::updateTile]: Partial at ({}, {})", tile->x(), tile->y());
        }

        m_texture_needs_update = true;
    }

    update();
}

bool RHIImageItem::textureNeedsUpdate() const
{
    QMutexLocker lock(&m_image_mutex);
    return m_texture_needs_update;
}

void RHIImageItem::setTextureNeedsUpdate(bool needs_update)
{
    QMutexLocker lock(&m_image_mutex);
    m_texture_needs_update = needs_update;
}

void RHIImageItem::onZoomChanged(float new_zoom)
{
    emit zoomChanged(new_zoom);
    update();
}

void RHIImageItem::onPanChanged(const QPointF& new_pan)
{
    emit panChanged(new_pan);
    update();
}

void RHIImageItem::onImageChanged()
{
    emit imageSizeChanged();
    update();
}

QQuickRhiItemRenderer* RHIImageItem::createRenderer()
{
    return new RHIImageItemRenderer(this);
}

} // namespace CaptureMoment::UI::Rendering
