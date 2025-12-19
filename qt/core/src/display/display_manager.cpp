/**
 * @file display_manager.cpp
 * @brief Implementation of DisplayManager
 */

#include "display/display_manager.h"
#include "rendering/i_rendering_item_base.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>

namespace CaptureMoment::UI::Display {

DisplayManager::DisplayManager(QObject* parent)
    : QObject(parent)
{
    spdlog::debug("DisplayManager: Created");
}

void DisplayManager::setRenderingItem(Rendering::IRenderingItemBase* item) {
    if (m_rendering_item == item) {
        return;
    }
    
    m_rendering_item = item;
    
    if (m_rendering_item) {
        spdlog::info("DisplayManager: Rendering item connected");
        m_rendering_item->setZoom(m_zoom);
        m_rendering_item->setPan(m_pan);
    }
}

void DisplayManager::createDisplayImage(const std::shared_ptr<ImageRegion>& sourceImage) {
    if (!sourceImage || !sourceImage->isValid()) {
        spdlog::warn("DisplayManager::createDisplayImage: Invalid source image");
        return;
    }
    
    QSize newSourceSize(sourceImage->m_width, sourceImage->m_height);
    if (m_source_image_size != newSourceSize) {
        m_source_image_size = newSourceSize;
        emit sourceImageSizeChanged(m_source_image_size);
    }
    
    QSize newDisplaySize = calculateDisplaySize(m_source_image_size, m_viewport_size);
    if (m_display_image_size != newDisplaySize) {
        m_display_image_size = newDisplaySize;
        emit displayImageSizeChanged(m_display_image_size);
    }
    
    float new_scale = static_cast<float>(m_display_image_size.width()) / m_source_image_size.width();
    if (!qFuzzyCompare(m_display_scale, new_scale)) {
        m_display_scale = new_scale;
        emit displayScaleChanged(m_display_scale);
    }
    
    spdlog::info("DisplayManager: Source {}x{} -> Display {}x{} (scale: {:.3f})",
                 m_source_image_size.width(), m_source_image_size.height(),
                 m_display_image_size.width(), m_display_image_size.height(),
                 m_display_scale);
    
    auto displayImage = downsampleImage(
        *sourceImage,
        m_display_image_size.width(),
        m_display_image_size.height()
    );
    
    if (displayImage && m_rendering_item) {
        m_rendering_item->setImage(displayImage);
        spdlog::debug("DisplayManager: Display image sent to rendering item");
    }
}

void DisplayManager::updateDisplayTile(const std::shared_ptr<ImageRegion>& sourceTile) {
    if (!sourceTile || !sourceTile->isValid()) {
        spdlog::warn("DisplayManager::updateDisplayTile: Invalid source tile");
        return;
    }
    
    if (!m_rendering_item) {
        spdlog::warn("DisplayManager::updateDisplayTile: No rendering item set");
        return;
    }
    
    int display_x = static_cast<int>(sourceTile->m_x * m_display_scale);
    int display_y = static_cast<int>(sourceTile->m_y * m_display_scale);
    int display_width = std::max(1, static_cast<int>(sourceTile->m_width * m_display_scale));
    int display_height = std::max(1, static_cast<int>(sourceTile->m_height * m_display_scale));
    
    spdlog::trace("DisplayManager: Tile ({},{}) {}x{} -> ({},{}) {}x{}",
                  sourceTile->m_x, sourceTile->m_y, sourceTile->m_width, sourceTile->m_height,
                  display_x, display_y, display_width, display_height);
    
    auto displayTile = downsampleImage(*sourceTile, display_width, display_height);
    
    if (displayTile) {
        displayTile->m_x = display_x;
        displayTile->m_y = display_y;
        m_rendering_item->updateTile(displayTile);
        spdlog::debug("DisplayManager: Display tile sent to rendering item");
    }
}

void DisplayManager::setZoom(float zoom) {
    zoom = std::clamp(zoom, 0.1f, 10.0f);
    
    if (!qFuzzyCompare(m_zoom, zoom)) {
        m_zoom = zoom;
        constrainPan();
        
        if (m_rendering_item) {
            m_rendering_item->setZoom(m_zoom);
        }
        
        emit zoomChanged(m_zoom);
    }
}

void DisplayManager::setPan(const QPointF& pan) {
    if (m_pan != pan) {
        m_pan = pan;
        constrainPan();
        
        if (m_rendering_item) {
            m_rendering_item->setPan(m_pan);
        }
        
        emit panChanged(m_pan);
    }
}

void DisplayManager::zoomAt(const QPointF& point, float zoomDelta) {
    float old_zoom = m_zoom;
    float new_zoom = std::clamp(old_zoom * zoomDelta, 0.1f, 10.0f);
    
    QPointF adjusted_pan = point - (point - m_pan) * (new_zoom / old_zoom);
    
    m_zoom = new_zoom;
    m_pan = adjusted_pan;
    constrainPan();
    
    if (m_rendering_item) {
        m_rendering_item->setZoom(m_zoom);
        m_rendering_item->setPan(m_pan);
    }
    
    emit zoomChanged(m_zoom);
    emit panChanged(m_pan);
}

void DisplayManager::fitToView() {
    m_zoom = 1.0f;
    m_pan = QPointF(0, 0);
    
    if (m_rendering_item) {
        m_rendering_item->setZoom(m_zoom);
        m_rendering_item->setPan(m_pan);
    }
    
    emit zoomChanged(m_zoom);
    emit panChanged(m_pan);
}

void DisplayManager::resetView() {
    fitToView();
}

void DisplayManager::setViewportSize(const QSize& size) {
    if (m_viewport_size != size && !size.isEmpty()) {
        m_viewport_size = size;
        
        if (!m_source_image_size.isEmpty()) {
            QSize newDisplaySize = calculateDisplaySize(m_source_image_size, m_viewport_size);
            if (m_display_image_size != newDisplaySize) {
                m_display_image_size = newDisplaySize;
                m_display_scale = static_cast<float>(m_display_image_size.width()) / m_source_image_size.width();
                
                emit displayImageSizeChanged(m_display_image_size);
                emit displayScaleChanged(m_display_scale);
                
                spdlog::debug("DisplayManager: Viewport resized, display size updated to {}x{}",
                             m_display_image_size.width(), m_display_image_size.height());
            }
        }
        
        constrainPan();
        emit viewportSizeChanged(size);
    }
}

QPointF DisplayManager::mapBackendToDisplay(int backendX, int backendY) const {
    return QPointF(backendX * m_display_scale, backendY * m_display_scale);
}

QPoint DisplayManager::mapDisplayToBackend(float displayX, float displayY) const {
    return QPoint(
        static_cast<int>(displayX / m_display_scale),
        static_cast<int>(displayY / m_display_scale)
    );
}

QSize DisplayManager::calculateDisplaySize(const QSize& sourceSize, const QSize& viewportSize) const {
    if (sourceSize.isEmpty() || viewportSize.isEmpty()) {
        return QSize();
    }
    
    float source_aspect = static_cast<float>(sourceSize.width()) / sourceSize.height();
    float viewport_aspect = static_cast<float>(viewportSize.width()) / viewportSize.height();
    
    int display_width, display_height;
    
    if (source_aspect > viewport_aspect) {
        display_width = viewportSize.width();
        display_height = static_cast<int>(display_width / source_aspect);
    } else {
        display_height = viewportSize.height();
        display_width = static_cast<int>(display_height * source_aspect);
    }
    
    return QSize(std::max(1, display_width), std::max(1, display_height));
}

std::shared_ptr<ImageRegion> DisplayManager::downsampleImage(
    const ImageRegion& source,
    int targetWidth,
    int targetHeight
) const {
    if (!source.isValid() || targetWidth <= 0 || targetHeight <= 0) {
        return nullptr;
    }
    
    if (source.m_width == targetWidth && source.m_height == targetHeight) {
        auto copy = std::make_shared<ImageRegion>();
        *copy = source;
        return copy;
    }
    
    auto downsampled = std::make_shared<ImageRegion>();
    downsampled->m_x = 0;
    downsampled->m_y = 0;
    downsampled->m_width = targetWidth;
    downsampled->m_height = targetHeight;
    downsampled->m_channels = source.m_channels;
    downsampled->m_format = source.m_format;
    downsampled->m_data.resize(targetWidth * targetHeight * source.m_channels);
    
    float x_ratio = static_cast<float>(source.m_width - 1) / targetWidth;
    float y_ratio = static_cast<float>(source.m_height - 1) / targetHeight;
    
    for (int y = 0; y < targetHeight; ++y) {
        for (int x = 0; x < targetWidth; ++x) {
            float src_x = x * x_ratio;
            float src_y = y * y_ratio;
            
            int x0 = static_cast<int>(src_x);
            int y0 = static_cast<int>(src_y);
            int x1 = std::min(x0 + 1, source.m_width - 1);
            int y1 = std::min(y0 + 1, source.m_height - 1);
            
            float fx = src_x - x0;
            float fy = src_y - y0;
            
            for (int c = 0; c < source.m_channels; ++c) {
                float v00 = source(y0, x0, c);
                float v10 = source(y0, x1, c);
                float v01 = source(y1, x0, c);
                float v11 = source(y1, x1, c);
                
                float v0 = v00 * (1.0f - fx) + v10 * fx;
                float v1 = v01 * (1.0f - fx) + v11 * fx;
                float value = v0 * (1.0f - fy) + v1 * fy;
                
                (*downsampled)(y, x, c) = value;
            }
        }
    }
    
    spdlog::trace("DisplayManager: Downsampled {}x{} -> {}x{}",
                  source.m_width, source.m_height,
                  targetWidth, targetHeight);
    
    return downsampled;
}

void DisplayManager::constrainPan() {
    if (m_display_image_size.isEmpty()) {
        return;
    }
    
    float visible_width = m_display_image_size.width() * m_zoom;
    float visible_height = m_display_image_size.height() * m_zoom;
    
    float max_pan_x = std::max(0.0f, (visible_width - m_viewport_size.width()) / 2.0f);
    float max_pan_y = std::max(0.0f, (visible_height - m_viewport_size.height()) / 2.0f);
    
    m_pan.setX(std::clamp(static_cast<float>(m_pan.x()), -max_pan_x, max_pan_x));
    m_pan.setY(std::clamp(static_cast<float>(m_pan.y()), -max_pan_y, max_pan_y));
}

} // namespace CaptureMoment::UI::Display
