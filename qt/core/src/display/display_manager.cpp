/**
 * @file display_manager.cpp
 * @brief Implementation of DisplayManager
 */

#include "display/display_manager.h"
#include "rendering/i_rendering_item_base.h"

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

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
    if (m_rendering_item == item) {
        spdlog::trace("DisplayManager::setRenderingItem: Item is already set, skipping update");
        return;
    }

    spdlog::debug("DisplayManager::setRenderingItem: Setting new rendering item");
    m_rendering_item = item;

    if (m_rendering_item) {
        spdlog::debug("DisplayManager::setRenderingItem: Updating zoom and pan on rendering item");
        m_rendering_item->setZoom(m_zoom);
        m_rendering_item->setPan(m_pan);
    } else {
        spdlog::warn("DisplayManager::setRenderingItem: Rendering item is null");
    }
}

void DisplayManager::createDisplayImage(const std::shared_ptr<Core::Common::ImageRegion>& source_image)
{
    if (!source_image) {
        spdlog::warn("DisplayManager::createDisplayImage: Source image is null");
        return;
    }

    if (!source_image->isValid()) {
        spdlog::warn("DisplayManager::createDisplayImage: Source image is invalid");
        return;
    }

    spdlog::debug("DisplayManager::createDisplayImage: Starting display image creation");

    // Keep reference for resizing
    m_source_image = source_image;

    QSize newSourceSize(source_image->m_width, source_image->m_height);
    if (m_source_image_size != newSourceSize) {
        m_source_image_size = newSourceSize;
        spdlog::debug("DisplayManager::createDisplayImage: Source image size updated to {}x{}",
                     m_source_image_size.width(), m_source_image_size.height());
        emit sourceImageSizeChanged(m_source_image_size);
    }

    QSize new_display_size = calculateDisplaySize(m_source_image_size, m_viewport_size);
    if (m_display_image_size != new_display_size) {
        m_display_image_size = new_display_size;
        spdlog::debug("DisplayManager::createDisplayImage: Display image size updated to {}x{}",
                     m_display_image_size.width(), m_display_image_size.height());
        emit displayImageSizeChanged(m_display_image_size);
    }

    float new_scale = static_cast<float>(m_display_image_size.width()) / m_source_image_size.width();
    if (!qFuzzyCompare(m_display_scale, new_scale)) {
        m_display_scale = new_scale;
        spdlog::debug("DisplayManager::createDisplayImage: Display scale updated to {:.6f}", m_display_scale);
        emit displayScaleChanged(m_display_scale);
    }

    spdlog::info("DisplayManager: Downsample {}x{} -> {}x{} (OIIO)",
                 m_source_image_size.width(), m_source_image_size.height(),
                 m_display_image_size.width(), m_display_image_size.height());

    std::shared_ptr<Core::Common::ImageRegion> displayImage;

    // Optimization: If the source image already matches the display size, use it directly
    // to avoid unnecessary downsampling and copying of pixel data.
    if (source_image->m_width == m_display_image_size.width() && source_image->m_height == m_display_image_size.height()) {
        displayImage = source_image; // Reuse the same image data, incrementing the reference count
        spdlog::debug("DisplayManager::createDisplayImage: Source matches display size, using direct reference");
    } else {
        // Perform downsampling using OIIO for optimal performance and quality
        displayImage = downsampleImage(*source_image, m_display_image_size.width(), m_display_image_size.height());
        if (!displayImage) {
            spdlog::error("DisplayManager::createDisplayImage: Downsampling failed");
            return;
        }
    }

    if (displayImage && m_rendering_item) {
        spdlog::debug("DisplayManager::createDisplayImage: Updating rendering item with display image");
        m_rendering_item->setImage(displayImage);
    } else {
        spdlog::warn("DisplayManager::createDisplayImage: Display image or rendering item is null");
    }
}

void DisplayManager::updateDisplayTile(const std::shared_ptr<Core::Common::ImageRegion>& source_tile)
{
    if (!source_tile) {
        spdlog::warn("DisplayManager::updateDisplayTile: Source tile is null");
        return;
    }

    if (!source_tile->isValid()) {
        spdlog::warn("DisplayManager::updateDisplayTile: Source tile is invalid");
        return;
    }

    if (!m_rendering_item) {
        spdlog::warn("DisplayManager::updateDisplayTile: No rendering item set");
        return;
    }

    spdlog::debug("DisplayManager::updateDisplayTile: Starting tile update");

    // Update reference
    m_source_image = source_tile;

    int display_width = m_display_image_size.width();
    int display_height = m_display_image_size.height();

    if (display_width <= 0 || display_height <= 0) {
        spdlog::warn("DisplayManager::updateDisplayTile: Invalid display dimensions {}x{}", display_width, display_height);
        return;
    }

    std::shared_ptr<Core::Common::ImageRegion> displayTile;

    // Optimization: If the source tile already matches the display size, use it directly
    // to avoid unnecessary downsampling and copying of pixel data.
    if (source_tile->m_width == display_width && source_tile->m_height == display_height) {
        displayTile = source_tile; // Reuse the same image data, incrementing the reference count
        spdlog::debug("DisplayManager::updateDisplayTile: Source tile matches display size, using direct reference");
    } else {
        // Perform downsampling using OIIO for optimal performance and quality
        displayTile = downsampleImage(*source_tile, display_width, display_height);
        if (!displayTile) {
            spdlog::error("DisplayManager::updateDisplayTile: Downsampling failed");
            return;
        }
    }

    if (displayTile) {
        displayTile->m_x = 0;
        displayTile->m_y = 0;
        spdlog::debug("DisplayManager::updateDisplayTile: Updating rendering item with tile");
        m_rendering_item->updateTile(displayTile);
    } else {
        spdlog::error("DisplayManager::updateDisplayTile: Generated display tile is null");
    }
}

void DisplayManager::setZoom(float zoom)
{
    float clamped_zoom = std::clamp(zoom, 0.1f, 10.0f);

    if (!qFuzzyCompare(m_zoom, clamped_zoom)) {
        spdlog::debug("DisplayManager::setZoom: Zoom changed from {:.6f} to {:.6f}", m_zoom, clamped_zoom);
        m_zoom = clamped_zoom;
        constrainPan();

        if (m_rendering_item) {
            spdlog::trace("DisplayManager::setZoom: Updating zoom on rendering item");
            m_rendering_item->setZoom(m_zoom);
        }

        emit zoomChanged(m_zoom);
    } else {
        spdlog::trace("DisplayManager::setZoom: Zoom value unchanged, skipping update");
    }
}

void DisplayManager::setPan(const QPointF& pan)
{
    if (m_pan != pan) {
        spdlog::debug("DisplayManager::setPan: Pan changed from ({:.6f}, {:.6f}) to ({:.6f}, {:.6f})",
                     m_pan.x(), m_pan.y(), pan.x(), pan.y());
        m_pan = pan;
        constrainPan();

        if (m_rendering_item) {
            spdlog::trace("DisplayManager::setPan: Updating pan on rendering item");
            m_rendering_item->setPan(m_pan);
        }

        emit panChanged(m_pan);
    } else {
        spdlog::trace("DisplayManager::setPan: Pan value unchanged, skipping update");
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

    spdlog::debug("DisplayManager::zoomAt: Zoom changed from {:.6f} to {:.6f}, pan adjusted to ({:.6f}, {:.6f})",
                 old_zoom, m_zoom, m_pan.x(), m_pan.y());

    if (m_rendering_item) {
        spdlog::trace("DisplayManager::zoomAt: Updating zoom and pan on rendering item");
        m_rendering_item->setZoom(m_zoom);
        m_rendering_item->setPan(m_pan);
    }

    emit zoomChanged(m_zoom);
    emit panChanged(m_pan);
}

void DisplayManager::fitToView()
{
    spdlog::debug("DisplayManager::fitToView: Fitting view to image");
    m_zoom = 1.0f;
    m_pan = QPointF(0, 0);

    if (m_rendering_item) {
        spdlog::trace("DisplayManager::fitToView: Updating zoom and pan on rendering item");
        m_rendering_item->setZoom(m_zoom);
        m_rendering_item->setPan(m_pan);
    }

    emit zoomChanged(m_zoom);
    emit panChanged(m_pan);
}

void DisplayManager::resetView() {
    spdlog::debug("DisplayManager::resetView: Resetting view");
    fitToView();
}

void DisplayManager::setViewportSize(const QSize& size) {
    if (m_viewport_size == size) {
        spdlog::trace("DisplayManager::setViewportSize: Viewport size unchanged, skipping update");
        return;
    }

    spdlog::debug("DisplayManager::setViewportSize: Viewport size changed from {}x{} to {}x{}",
                 m_viewport_size.width(), m_viewport_size.height(), size.width(), size.height());
    m_viewport_size = size;

    if (!m_source_image_size.isEmpty() && m_source_image) {
        QSize new_display_size = calculateDisplaySize(m_source_image_size, m_viewport_size);
        if (m_display_image_size != new_display_size) {
            m_display_image_size = new_display_size;
            m_display_scale = static_cast<float>(m_display_image_size.width()) / m_source_image_size.width();

            spdlog::info("DisplayManager: Viewport resize - downsample {}x{} -> {}x{} (OIIO)",
                         m_source_image_size.width(), m_source_image_size.height(),
                         m_display_image_size.width(), m_display_image_size.height());

            // Re-downsample source image to fit new viewport size using OIIO
            auto newDisplayImage = downsampleImage(*m_source_image, m_display_image_size.width(), m_display_image_size.height());
            if (newDisplayImage && m_rendering_item) {
                spdlog::debug("DisplayManager::setViewportSize: Updating rendering item with resized display image");
                m_rendering_item->setImage(newDisplayImage);
            } else {
                spdlog::warn("DisplayManager::setViewportSize: Failed to generate or update display image after viewport resize");
            }

            emit displayImageSizeChanged(m_display_image_size);
            emit displayScaleChanged(m_display_scale);
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
        spdlog::warn("DisplayManager::calculateDisplaySize: Invalid input sizes");
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

    spdlog::debug("DisplayManager::calculateDisplaySize: Calculated display size {}x{} from source {}x{} and viewport {}x{}",
                 display_width, display_height, source_size.width(), source_size.height(),
                 viewport_size.width(), viewport_size.height());

    return QSize(display_width, display_height);
}

std::shared_ptr<Core::Common::ImageRegion> DisplayManager::downsampleImage(
    const Core::Common::ImageRegion& source,
    int target_width,
    int target_height
    ) const
{
    if (!source.isValid()) {
        spdlog::error("DisplayManager::downsampleImage: Source image region is invalid");
        return nullptr;
    }

    if (target_width <= 0 || target_height <= 0) {
        spdlog::error("DisplayManager::downsampleImage: Invalid target dimensions {}x{}", target_width, target_height);
        return nullptr;
    }

    spdlog::debug("DisplayManager::downsampleImage: Starting downsampling from {}x{} to {}x{}",
                 source.m_width, source.m_height, target_width, target_height);

    // Note: Size matching optimization should be handled by the caller
    // to avoid unnecessary reference counting overhead when sizes match.
    // This function assumes downsampling is actually needed.

    // -------------------------------------------------------------------
    // OIIO MAGIC WITH RESAMPLE
    // -------------------------------------------------------------------

    // 1. Create OIIO Spec and Buffer wrapping Source Data (Zero-Copy)
    OIIO::ImageSpec src_spec(source.m_width, source.m_height, source.m_channels, OIIO::TypeDesc::FLOAT);
    OIIO::ImageBuf src_buf(src_spec, const_cast<float*>(source.m_data.data()));

    // 2. Define the ROI for the target size
    OIIO::ROI roi(0, target_width, 0, target_height, 0, 1, 0, source.m_channels);

    // 3. Perform Resample (Fast, bilinear interpolation)
    OIIO::ImageBuf result_buf = OIIO::ImageBufAlgo::resample(src_buf, true, roi);

    if (!result_buf.initialized()) {
        spdlog::error("DisplayManager::downsampleImage: OIIO resample failed (buffer not initialized)");
        return nullptr;
    }

    // 4. Extract result back to our ImageRegion structure
    auto downsampled = std::make_shared<Core::Common::ImageRegion>();
    downsampled->m_x = 0;
    downsampled->m_y = 0;
    downsampled->m_width = target_width;
    downsampled->m_height = target_height;
    downsampled->m_channels = source.m_channels;
    downsampled->m_format = source.m_format;

    const size_t total_pixels = static_cast<size_t>(target_width) * target_height * source.m_channels;
    downsampled->m_data.resize(total_pixels);

    // Direct extraction to the vector
    if (!result_buf.get_pixels(OIIO::ROI::All(), OIIO::TypeDesc::FLOAT, downsampled->m_data.data())) {
        spdlog::error("DisplayManager::downsampleImage: Failed to extract pixels from OIIO buffer");
        return nullptr;
    }

    spdlog::debug("DisplayManager::downsampleImage: Successfully created downsampled image {}x{}",
                 downsampled->m_width, downsampled->m_height);

    return downsampled;
}

void DisplayManager::constrainPan()
{
    if (m_display_image_size.isEmpty()) {
        spdlog::trace("DisplayManager::constrainPan: Display image size is empty, skipping constraint");
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
        spdlog::debug("DisplayManager::constrainPan: Pan constrained from ({:.6f}, {:.6f}) to ({:.6f}, {:.6f})",
                     old_pan.x(), old_pan.y(), m_pan.x(), m_pan.y());
    }
}

} // namespace CaptureMoment::UI::Display
