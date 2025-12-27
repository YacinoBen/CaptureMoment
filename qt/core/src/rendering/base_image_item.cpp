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
BaseImageItem::BaseImageItem(QQuickItem* parent)
    : QQuickItem(parent)
{
    // Indicate to Qt Quick that this item has custom content rendered via the scene graph.
    setFlag(QQuickItem::ItemHasContents, true); // If all derived classes need this
    spdlog::debug("BaseImageItem: Created");
}

// Sets the zoom level (default implementation).
void BaseImageItem::setZoom(float zoom) {
    if (!qFuzzyCompare(m_zoom, zoom)) {
        m_zoom = zoom;
        emit zoomChanged(m_zoom);
        // update(); // Do not call update() here, derived classes should do it if needed
    }
}

// Sets the pan offset (default implementation).
void BaseImageItem::setPan(const QPointF& pan) {
    if (m_pan != pan) {
        m_pan = pan;
        emit panChanged(m_pan);
        // update(); // Do not call update() here, derived classes should do it if needed
    }
}

// Gets the width of the image.
int BaseImageItem::imageWidth() const
{
    QMutexLocker lock(&m_image_mutex);
    return m_image_width;
}

// Gets the height of the image.
int BaseImageItem::imageHeight() const
{
    QMutexLocker lock(&m_image_mutex);
    return m_image_height;
}
} // namespace CaptureMoment::UI::Rendering
