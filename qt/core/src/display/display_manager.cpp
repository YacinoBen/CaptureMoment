/**
 * @file display_manager.cpp
 * @brief Implementation of DisplayManager
 */

#include "display/display_manager.h"
#include "rendering/i_rendering_item_base.h"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace CaptureMoment::UI::Display {

DisplayManager::DisplayManager(QObject* parent)
    : QObject(parent)
{
    spdlog::debug("DisplayManager::DisplayManager Created");
}

void DisplayManager::setRenderingItem(Rendering::IRenderingItemBase* item)
{
    spdlog::info("DisplayManager::setRenderingItem: RHIItem/Painted/SGS item connected: {}", m_rendering_item ? "valid" : "null");

    if (m_rendering_item == item) {
        return;
    }
    
    m_rendering_item = item;

    spdlog::info("DisplayManager::setRenderingItem: m_rendering_item assigned, value is now {}", m_rendering_item ? "valid" : "null");
    
    if (m_rendering_item) {
        spdlog::info("DisplayManager::setRenderingItem Rendering item connected");
        m_rendering_item->setZoom(m_zoom);
        m_rendering_item->setPan(m_pan);
    }
}

void DisplayManager::createDisplayImage(const std::shared_ptr<Core::Common::ImageRegion>& source_image)
{
    if (!source_image || !source_image->isValid()) {
        spdlog::warn("DisplayManager::createDisplayImage: Invalid source image");
        return;
    }

    m_source_image = source_image;

    QSize newSourceSize(source_image->m_width, source_image->m_height);
    if (m_source_image_size != newSourceSize) {
        m_source_image_size = newSourceSize;
        emit sourceImageSizeChanged(m_source_image_size);
    }

    QSize new_display_size = calculateDisplaySize(m_source_image_size, m_viewport_size);
    if (m_display_image_size != new_display_size)
    {
        m_display_image_size = new_display_size;
        emit displayImageSizeChanged(m_display_image_size);
    }

    float new_scale = static_cast<float>(m_display_image_size.width()) / m_source_image_size.width();
    if (!qFuzzyCompare(m_display_scale, new_scale))
    {
        m_display_scale = new_scale;
        emit displayScaleChanged(m_display_scale);
    }

    spdlog::info("DisplayManager::createDisplayImage: Source {}x{} -> Display {}x{} (scale: {:.6f})",
                 m_source_image_size.width(), m_source_image_size.height(),
                 m_display_image_size.width(), m_display_image_size.height(),
                 m_display_scale);

    // DOWNSAMPLE
    auto displayImage = downsampleImage(
        *source_image,
        m_display_image_size.width(),
        m_display_image_size.height()
        );

    if (displayImage && m_rendering_item)
    {
        m_rendering_item->setImage(displayImage);
        spdlog::info("DisplayManager::createDisplayImage: Display image set to rendering item");
    }
}

void DisplayManager::updateDisplayTile(const std::shared_ptr<Core::Common::ImageRegion>& source_tile)
{
    if (!source_tile || !source_tile->isValid()) {
        spdlog::warn("DisplayManager::updateDisplayTile: Invalid source tile");
        return;
    }

    if (!m_rendering_item) {
        spdlog::warn("DisplayManager::updateDisplayTile: No rendering item set");
        return;
    }

    m_source_image = source_tile;

    int display_width = m_display_image_size.width();
    int display_height = m_display_image_size.height();

    if (display_width <= 0 || display_height <= 0)
    {
        spdlog::warn("DisplayManager::updateDisplayTile: Invalid display size {}x{}",
                     display_width, display_height);
        return;
    }

    spdlog::info("DisplayManager::updateDisplayTile: Downsampling {}x{} to EXACT {}x{}",
                 source_tile->m_width, source_tile->m_height,
                 display_width, display_height);

    // Downsample
    auto displayTile = downsampleImage(
        *source_tile,
        display_width,
        display_height
        );

    if (displayTile)
    {
        displayTile->m_x = 0;
        displayTile->m_y = 0;
        spdlog::info("DisplayManager::updateDisplayTile: Created display tile {}x{} (expected {}x{})",
                     displayTile->m_width, displayTile->m_height,
                     display_width, display_height);

        if (displayTile->m_width != display_width || displayTile->m_height != display_height) {
            spdlog::error("DisplayManager::updateDisplayTile: SIZE MISMATCH! "
                          "Expected {}x{}, got {}x{}",
                          display_width, display_height,
                          displayTile->m_width, displayTile->m_height);
        }

        m_rendering_item->updateTile(displayTile);
    } else {
        spdlog::error("DisplayManager::updateDisplayTile: downsampleImage returned nullptr");
    }
}

void DisplayManager::setZoom(float zoom)
{
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

void DisplayManager::setPan(const QPointF& pan)
{
    if (m_pan != pan)
    {
        m_pan = pan;
        constrainPan();
        
        if (m_rendering_item) {
            m_rendering_item->setPan(m_pan);
        }
        
        emit panChanged(m_pan);
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
    
    if (m_rendering_item) {
        m_rendering_item->setZoom(m_zoom);
        m_rendering_item->setPan(m_pan);
    }
    
    emit zoomChanged(m_zoom);
    emit panChanged(m_pan);
}

void DisplayManager::fitToView()
{
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

    if (!m_source_image_size.isEmpty() && m_source_image)
    {
        QSize new_display_size = calculateDisplaySize(m_source_image_size, m_viewport_size);
        if (m_display_image_size != new_display_size)
        {
            m_display_image_size = new_display_size;
            m_display_scale = static_cast<float>(m_display_image_size.width()) / m_source_image_size.width();

            auto newDisplayImage = downsampleImage(
                *m_source_image,
                m_display_image_size.width(),
                m_display_image_size.height()
                );
            if (newDisplayImage && m_rendering_item) {
                m_rendering_item->setImage(newDisplayImage);
            }

            emit displayImageSizeChanged(m_display_image_size);
            emit displayScaleChanged(m_display_scale);
        }
    }
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
    if (source_size.isEmpty() || viewport_size.isEmpty())
    {
        spdlog::warn("DisplayManager::calculateDisplaySize: source_size.isEmpty() || viewport_size.isEmpty() false");
        return QSize();
    }

    float source_aspect = static_cast<float>(source_size.width()) / source_size.height();
    float viewport_aspect = static_cast<float>( viewport_size.width()) /  viewport_size.height();

    int display_width, display_height;

    if (source_aspect > viewport_aspect)
    {
        display_width =  viewport_size.width();
        display_height = static_cast<int>(display_width / source_aspect);
    } else
    {
        display_height =  viewport_size.height();
        display_width = static_cast<int>(display_height * source_aspect);
    }

    // ✅ S'assurer que les dimensions sont valides
    display_width = std::max(1, display_width);
    display_height = std::max(1, display_height);

    spdlog::info("DisplayManager::calculateDisplaySize: Source {}x{}, Viewport {}x{} -> Display {}x{}",
                 source_size.width(), source_size.height(),
                  viewport_size.width(),  viewport_size.height(),
                 display_width, display_height);

    return QSize(display_width, display_height);
}

std::shared_ptr<Core::Common::ImageRegion> DisplayManager::downsampleImage(
    const Core::Common::ImageRegion& source,
    int target_width,
    int target_height
) const
{
    if (!source.isValid() || target_width <= 0 || target_height <= 0) {
        return nullptr;
    }
    
    if (source.m_width == target_width && source.m_height == target_height) {
        auto copy = std::make_shared<Core::Common::ImageRegion>();
        *copy = source;
        return copy;
    }
    
    auto downsampled = std::make_shared<Core::Common::ImageRegion>();
    downsampled->m_x = 0;
    downsampled->m_y = 0;
    downsampled->m_width = target_width;
    downsampled->m_height = target_height;
    downsampled->m_channels = source.m_channels;
    downsampled->m_format = source.m_format;
    downsampled->m_data.resize(target_width * target_height * source.m_channels);
    
    float x_ratio = static_cast<float>(source.m_width - 1) / target_width;
    float y_ratio = static_cast<float>(source.m_height - 1) / target_height;
    
    spdlog::info("downsampleImage: Creating {}x{} from {}x{}",
                 target_width, target_height,
                 source.m_width, source.m_height);

    int last_src_idx = ((source.m_height - 1) * source.m_width + 0) * source.m_channels;
    if (last_src_idx + 2 < source.m_data.size())
    {
        spdlog::info("downsampleImage: Last source pixel (0, {}): {:.3f},{:.3f},{:.3f}",
                     source.m_height - 1,
                     source.m_data[last_src_idx],
                     source.m_data[last_src_idx+1],
                     source.m_data[last_src_idx+2]);
    }

    for (int y = 0; y < target_height; ++y)
    {
        for (int x = 0; x < target_width; ++x)
        {
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

    spdlog::info("downsampleImage: Last downsampled pixel (0, {}): {:.3f},{:.3f},{:.3f}",
                 target_height - 1,
                 downsampled->m_data[((target_height-1) * target_width + 0) * source.m_channels],
                 downsampled->m_data[((target_height-1) * target_width + 0) * source.m_channels + 1],
                 downsampled->m_data[((target_height-1) * target_width + 0) * source.m_channels + 2]);
    
    spdlog::trace("DisplayManager: Downsampled {}x{} -> {}x{}",
                  source.m_width, source.m_height,
                  target_width, target_height);
    
    return downsampled;
}

void DisplayManager::constrainPan()
{
    if (m_display_image_size.isEmpty()) {
        spdlog::error("DisplayManager::constrainPan: m_display_image_size is empty");
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
