/**
 * @file painted_image_item.cpp
 * @brief Implementation of PaintedImageItem using QQuickPaintedItem and QPainter
 * @author CaptureMoment Team
 * @date 2025
 */

#include "rendering/painted_image_item.h"
#include <spdlog/spdlog.h>
#include <QPainter>
#include <QMutexLocker>
#include <algorithm>

namespace CaptureMoment::UI::Rendering {

PaintedImageItem::PaintedImageItem(QQuickItem* parent)
    : QQuickPaintedItem(parent)
{
    spdlog::debug("[PaintedImageItem:aintedImageItem]: Created");
}

// Sets the full image to be displayed.
// Updates the internal image data and schedules a repaint.
void PaintedImageItem::setImage(std::unique_ptr<Core::Common::ImageRegion> image)
{
    if (!image || !image->isValid()) {
        spdlog::warn("[PaintedImageItem::setImage]: Invalid image");
        return;
    }

    spdlog::info("[PaintedImageItem::setImage]: {}x{}", image->width(), image->height());

    {
        QMutexLocker lock(&m_image_mutex);
        m_full_image = std::move(image);
        m_current_qimage = convertImageRegionToQImage(*m_full_image);
    }

    onImageChanged();
}

void PaintedImageItem::updateTile(std::unique_ptr<Core::Common::ImageRegion> tile)
{
    if (!tile || !tile->isValid()) {
        spdlog::warn("[PaintedImageItem::updateTile]: Invalid tile");
        return;
    }

    const int tile_w { static_cast<int>(tile->width()) };
    const int tile_h { static_cast<int>(tile->height()) };

    {
        QMutexLocker lock(&m_image_mutex);

        // ALWAYS full replacement - we receive the complete downsampled image
        // The name "updateTile" is misleading; it's always the full display image
        m_full_image = std::move(tile);
        m_current_qimage = convertImageRegionToQImage(*m_full_image);
    }

    spdlog::debug("[PaintedImageItem::updateTile]: Replaced with {}x{}", tile_w, tile_h);

    // Thread-safe update - must be called from GUI thread
    QMetaObject::invokeMethod(this, &QQuickItem::update, Qt::QueuedConnection);
}

// Paints the image using QPainter.
void PaintedImageItem::paint(QPainter* painter)
{
    if (!painter || !isImagePaintValid()) {
        return;
    }

    QMutexLocker lock(&m_image_mutex);

    painter->save();
    painter->translate(m_pan);
    painter->scale(m_zoom, m_zoom);
    painter->drawImage(QPointF(0, 0), m_current_qimage);
    painter->restore();
}

// Converts an ImageRegion to a QImage.
QImage PaintedImageItem::convertImageRegionToQImage(const Core::Common::ImageRegion& region) const
{
    if (!region.isValid()) {
        spdlog::error("[PaintedImageItem::convertImageRegionToQImage]: Invalid region");
        return QImage();
    }

    const int w { static_cast<int>(region.width()) };
    const int h { static_cast<int>(region.height()) };
    const int ch { region.channels() };

    QImage::Format format { (ch == 4) ? QImage::Format_RGBA8888 : QImage::Format_RGB888 };
    QImage qimg(w, h, format);

    const float* src { region.m_data.data() };
    uchar* dst = qimg.bits();

    for (int i = 0; i < w * h * ch; ++i) {
        dst[i] = static_cast<uchar>(std::clamp(src[i], 0.0f, 1.0f) * 255.0f);
    }

    return qimg;
}

bool PaintedImageItem::isImagePaintValid() const
{
    QMutexLocker lock(&m_image_mutex);
    return m_full_image && m_full_image->isValid() && !m_current_qimage.isNull();
}


void PaintedImageItem::onZoomChanged(float new_zoom)
{
    emit zoomChanged(new_zoom);
    update();
}

void PaintedImageItem::onPanChanged(const QPointF& new_pan)
{
    emit panChanged(new_pan);
    update();
}

void PaintedImageItem::onImageChanged()
{
    emit imageSizeChanged();
    update();
}
} // namespace CaptureMoment::UI::Rendering
