/**
 * @file base_image_item.cpp
 * @brief Implementation of BaseImageItem
 * @author CaptureMoment Team
 * @date 2025
 */

#include "rendering/base_image_item.h"
#include <spdlog/spdlog.h>

namespace CaptureMoment::UI::Rendering {

// Constructor: Initializes the base item.
BaseImageItem::BaseImageItem()
{
    spdlog::debug("[BaseImageItem::BaseImageItem]: Created");
}

void BaseImageItem::setZoom(float zoom)
{
    if (!qFuzzyCompare(m_zoom, zoom) && zoom > 0.0f) {
        m_zoom = zoom;
        onZoomChanged(zoom);
    }
}

void BaseImageItem::setPan(const QPointF& pan)
{
    if (m_pan != pan) {
        m_pan = pan;
        onPanChanged(pan);
    }
}

bool BaseImageItem::isImageValid() const
{
    QMutexLocker lock(&m_image_mutex);
    return m_full_image && m_full_image->isValid();
}

// Gets the width of the image.
int BaseImageItem::imageWidth() const
{
    QMutexLocker lock(&m_image_mutex);
    return m_full_image ? static_cast<int>(m_full_image->width()) : 0;
}

// Gets the height of the image.
int BaseImageItem::imageHeight() const
{
    QMutexLocker lock(&m_image_mutex);
    return m_full_image ? static_cast<int>(m_full_image->height()) : 0;
}

} // namespace CaptureMoment::UI::Rendering
