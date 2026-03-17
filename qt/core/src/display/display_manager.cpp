/**
 * @file display_manager.cpp
 * @brief Implementation of DisplayManager
 * @author CaptureMoment Team
 * @date 2025
 */

#include "display/display_manager.h"
#include "rendering/i_rendering_item_base.h"

#include <spdlog/spdlog.h>
#include <algorithm>

namespace CaptureMoment::UI::Display {

DisplayManager::DisplayManager(QObject* parent)
    : QObject(parent)
{
    spdlog::debug("[DisplayManager::DisplayManager]: Created");
}

void DisplayManager::setRenderingItem(Rendering::IRenderingItemBase* item)
{
    if (m_rendering_item == item) {
        spdlog::trace("[DisplayManager::setRenderingItem]: Item is already set, skipping update");
        return;
    }

    spdlog::debug("[DisplayManager::setRenderingItem]: Setting new rendering item");
    m_rendering_item = item;

    if (m_rendering_item) {
        spdlog::debug("[DisplayManager::setRenderingItem]: Updating zoom and pan on rendering item");
        m_rendering_item->setZoom(m_zoom);
        m_rendering_item->setPan(m_pan);
    } else {
        spdlog::warn("[DisplayManager::setRenderingItem]: Rendering item is null");
    }
}

void DisplayManager::createDisplayImage(std::unique_ptr<Core::Common::ImageRegion> source_image)
{
    if (!source_image || !source_image->isValid()) {
        spdlog::warn("[DisplayManager::createDisplayImage]: Invalid source image");
        return;
    }

    if (!m_rendering_item) {
        spdlog::warn("[DisplayManager::createDisplayImage]: No rendering item");
        return;
    }

    // UPDATE display size HERE - when we actually receive the image
    m_display_image_size = QSize(source_image->width(), source_image->height());
    m_display_scale = static_cast<float>(m_display_image_size.width()) / m_source_image_size.width();

    spdlog::debug("[DisplayManager::createDisplayImage]: Creating display {}x{}",
                  m_display_image_size.width(), m_display_image_size.height());

    m_rendering_item->setImage(std::move(source_image));

    emit displayImageSizeChanged(m_display_image_size);
    emit displayScaleChanged(m_display_scale);
}

void DisplayManager::updateDisplayTile(std::unique_ptr<Core::Common::ImageRegion> source_tile)
{
    if (!source_tile || !source_tile->isValid()) {
        spdlog::warn("[DisplayManager::updateDisplayTile]: Invalid source tile");
        return;
    }

    if (!m_rendering_item) {
        spdlog::warn("[DisplayManager::updateDisplayTile]: No rendering item");
        return;
    }

    // UPDATE display size HERE - when we actually receive the image
    m_display_image_size = QSize(source_tile->width(), source_tile->height());
    m_display_scale = static_cast<float>(m_display_image_size.width()) / m_source_image_size.width();

    spdlog::debug("[DisplayManager::updateDisplayTile]: Updating display {}x{}",
                  m_display_image_size.width(), m_display_image_size.height());

    m_rendering_item->updateTile(std::move(source_tile));

    emit displayImageSizeChanged(m_display_image_size);
    emit displayScaleChanged(m_display_scale);
}

void DisplayManager::setSourceImageSize(int width, int height)
{
    m_source_image_size = QSize(width, height);
    spdlog::debug("[DisplayManager::setSourceImageSize]: {}x{}", width, height);

    // Recalculate display size based on viewport
    if (!m_viewport_size.isEmpty()) {
        QSize new_display_size = calculateDisplaySize(m_source_image_size, m_viewport_size);
        if (m_display_image_size != new_display_size) {
            m_display_image_size = new_display_size;
            m_display_scale = static_cast<float>(m_display_image_size.width()) / m_source_image_size.width();
            emit displayImageSizeChanged(m_display_image_size);
            emit displayScaleChanged(m_display_scale);
        }
    }

    emit sourceImageSizeChanged(m_source_image_size);
}

void DisplayManager::setZoom(float zoom)
{
    float clamped_zoom = std::clamp(zoom, 0.1f, 10.0f);

    if (!qFuzzyCompare(m_zoom, clamped_zoom)) {
        spdlog::debug("[DisplayManager::setZoom]: Zoom changed from {:.6f} to {:.6f}", m_zoom, clamped_zoom);
        m_zoom = clamped_zoom;
        constrainPan();

        if (m_rendering_item) {
            spdlog::trace("[DisplayManager::setZoom]: Updating zoom on rendering item");
            m_rendering_item->setZoom(m_zoom);
        }

        emit zoomChanged(m_zoom);
    } else {
        spdlog::trace("[DisplayManager::setZoom]: Zoom value unchanged, skipping update");
    }
}

void DisplayManager::setPan(const QPointF& pan)
{
    if (m_pan != pan) {
        spdlog::debug("[DisplayManager::setPan]: Pan changed from ({:.6f}, {:.6f}) to ({:.6f}, {:.6f})",
                     m_pan.x(), m_pan.y(), pan.x(), pan.y());
        m_pan = pan;
        constrainPan();

        if (m_rendering_item) {
            spdlog::trace("[DisplayManager::setPan]: Updating pan on rendering item");
            m_rendering_item->setPan(m_pan);
        }

        emit panChanged(m_pan);
    } else {
        spdlog::trace("[DisplayManager::setPan]: Pan value unchanged, skipping update");
    }
}

void DisplayManager::zoomAt(const QPointF& point, float zoom_delta)
{
    float old_zoom = m_zoom;
    float new_zoom = std::clamp(old_zoom * zoom_delta, 0.1f, 10.0f);

    QPointF adjusted_pan = point - (point - m_pan) * (new_zoom / old_zoom);

    m_zoom = new_zoom;
    m_pan = adjusted_pan;
    constrainPan();

    spdlog::debug("[DisplayManager::zoomAt]: Zoom changed from {:.6f} to {:.6f}, pan adjusted to ({:.6f}, {:.6f})",
                 old_zoom, m_zoom, m_pan.x(), m_pan.y());

    if (m_rendering_item) {
        spdlog::trace("[DisplayManager::zoomAt]: Updating zoom and pan on rendering item");
        m_rendering_item->setZoom(m_zoom);
        m_rendering_item->setPan(m_pan);
    }

    emit zoomChanged(m_zoom);
    emit panChanged(m_pan);
}

void DisplayManager::fitToView()
{
    spdlog::debug("[DisplayManager::fitToView]: Fitting view to image");
    m_zoom = 1.0f;
    m_pan = QPointF(0, 0);

    if (m_rendering_item) {
        spdlog::trace("[DisplayManager::fitToView]: Updating zoom and pan on rendering item");
        m_rendering_item->setZoom(m_zoom);
        m_rendering_item->setPan(m_pan);
    }

    emit zoomChanged(m_zoom);
    emit panChanged(m_pan);
}

void DisplayManager::resetView() {
    spdlog::debug("[DisplayManager::resetView]: Resetting view");
    fitToView();
}

void DisplayManager::setViewportSize(const QSize& size) {
    if (m_viewport_size == size) {
        return;
    }

    spdlog::debug("[DisplayManager::setViewportSize]: {}x{}", size.width(), size.height());
    m_viewport_size = size;

    // Recalculate display size if we have source info
    if (!m_source_image_size.isEmpty()) {
        QSize new_display_size = calculateDisplaySize(m_source_image_size, m_viewport_size);

        // Only emit request if size actually changed
        if (new_display_size != m_display_image_size) {
            // DON'T update m_display_image_size here!
            // Wait until we actually receive the new image

            spdlog::debug("[DisplayManager::setViewportSize]: Requesting new image {}x{} (current is {}x{})",
                          new_display_size.width(), new_display_size.height(),
                          m_display_image_size.width(), m_display_image_size.height());

            emit displayImageRequest(new_display_size.width(), new_display_size.height());
        }
    }

    emit viewportSizeChanged(size);
}

QPointF DisplayManager::mapBackendToDisplay(int backend_x, int backend_y) const
{
    return QPointF(backend_x * m_display_scale, backend_y * m_display_scale);
}

QPoint DisplayManager::mapDisplayToBackend(float display_x, float display_y) const
{
    return QPoint(
        static_cast<int>(display_x / m_display_scale),
        static_cast<int>(display_y / m_display_scale)
    );
}

QSize DisplayManager::calculateDisplaySize(const QSize& source_size, const QSize& viewport_size) const
{
    if (source_size.isEmpty() || viewport_size.isEmpty()) {
        spdlog::warn("[DisplayManager::calculateDisplaySize]: Invalid input sizes");
        return QSize();
    }

    float source_aspect = static_cast<float>(source_size.width()) / source_size.height();
    float viewport_aspect = static_cast<float>(viewport_size.width()) / viewport_size.height();

    int display_width, display_height;

    if (source_aspect > viewport_aspect) {
        display_width = viewport_size.width();
        display_height = static_cast<int>(display_width / source_aspect);
    } else {
        display_height = viewport_size.height();
        display_width = static_cast<int>(display_height * source_aspect);
    }

    display_width = std::max(1, display_width);
    display_height = std::max(1, display_height);

    spdlog::debug("[DisplayManager::calculateDisplaySize]: Calculated display size {}x{} from source {}x{} and viewport {}x{}",
                 display_width, display_height, source_size.width(), source_size.height(),
                 viewport_size.width(), viewport_size.height());

    return QSize(display_width, display_height);
}

void DisplayManager::constrainPan()
{
    if (m_display_image_size.isEmpty()) {
        spdlog::trace("[DisplayManager::constrainPan]: Display image size is empty, skipping constraint");
        return;
    }

    float visible_width = m_display_image_size.width() * m_zoom;
    float visible_height = m_display_image_size.height() * m_zoom;

    float max_pan_x = std::max(0.0f, (visible_width - m_viewport_size.width()) / 2.0f);
    float max_pan_y = std::max(0.0f, (visible_height - m_viewport_size.height()) / 2.0f);

    QPointF old_pan = m_pan;
    m_pan.setX(std::clamp(static_cast<float>(m_pan.x()), -max_pan_x, max_pan_x));
    m_pan.setY(std::clamp(static_cast<float>(m_pan.y()), -max_pan_y, max_pan_y));

    if (m_pan != old_pan) {
        spdlog::debug("[DisplayManager::constrainPan]: Pan constrained from ({:.6f}, {:.6f}) to ({:.6f}, {:.6f})",
                     old_pan.x(), old_pan.y(), m_pan.x(), m_pan.y());
    }
}

} // namespace CaptureMoment::UI::Display
