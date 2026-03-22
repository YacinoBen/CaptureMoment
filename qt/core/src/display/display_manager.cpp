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
#include <QScreen>

namespace CaptureMoment::UI::Display {

DisplayManager::DisplayManager(QObject* parent)
    : QObject(parent)
    , m_viewport_manager(std::make_unique<ViewportManager>())
{
    spdlog::debug("[DisplayManager::DisplayManager]: Created");
    initialize();
}

// =============================================================================
// Initialization
// =============================================================================

void DisplayManager::initialize()
{
    QScreen* primary_screen = QGuiApplication::primaryScreen();

    ScreenInfo screen_info;
    QSize viewport_default(800, 600);

    if (primary_screen) {
        QSize physical = primary_screen->size() * primary_screen->devicePixelRatio();
        screen_info.physical_width = physical.width();
        screen_info.physical_height = physical.height();
        screen_info.dpr = static_cast<float>(primary_screen->devicePixelRatio());

        spdlog::info("[DisplayManager::initialize]: Auto-detected screen: {}x{}, DPR: {:.2f}",
                     screen_info.physical_width, screen_info.physical_height, screen_info.dpr);
    } else {
        spdlog::warn("[DisplayManager::initialize]: No screen detected, using defaults");
        screen_info.physical_width = 1920;
        screen_info.physical_height = 1080;
        screen_info.dpr = 1.0f;
    }

    initialize(viewport_default, screen_info);
}

void DisplayManager::initialize(const QSize& viewport_logical, const ScreenInfo& screen)
{
    // =========================================================================
    // Validate inputs
    // =========================================================================

    if (!viewport_logical.isValid() || viewport_logical.isEmpty()) {
        spdlog::error("[DisplayManager::initialize]: Invalid viewport size");
        return;
    }

    if (!screen.isValid()) {
        spdlog::error("[DisplayManager::initialize]: Invalid screen info");
        return;
    }

    // =========================================================================
    // Initialize viewport manager
    // =========================================================================

    m_viewport_manager->initialize(viewport_logical, screen);

    // =========================================================================
    // Log configuration
    // =========================================================================

    spdlog::info("[DisplayManager::initialize]: Initialized successfully");
    spdlog::info("[DisplayManager::initialize]: Max downsample: {}px", m_viewport_manager->maxDownsample());
    spdlog::info("[DisplayManager::initialize]: Plafond: {}px", m_viewport_manager->plafond());
    spdlog::info("[DisplayManager::initialize]: Quality margin: {:.2f}", m_viewport_manager->qualityMargin());

    emit initializedChanged(true);
}

bool DisplayManager::isInitialized() const noexcept
{
    return m_viewport_manager && m_viewport_manager->isInitialized();
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
    // =========================================================================
    // Validate input
    // =========================================================================

    if (!source_image || !source_image->isValid()) {
        spdlog::warn("[DisplayManager::createDisplayImage]: Invalid source image");
        return;
    }

    if (!m_rendering_item) {
        spdlog::warn("[DisplayManager::createDisplayImage]: No rendering item");
        return;
    }

    // =========================================================================
    // Update downsample size from received image
    // =========================================================================

    m_downsample_size = QSize(source_image->width(), source_image->height());

    // =========================================================================
    // Calculate display scale (ratio of downsample to source)
    // =========================================================================

    if (m_source_image_size.isValid() && m_source_image_size.width() > 0) {
        m_display_scale = static_cast<float>(m_downsample_size.width()) /
                          static_cast<float>(m_source_image_size.width());
    }

    spdlog::debug("[DisplayManager::createDisplayImage]: Creating display image: {}x{}, scale={:.3f}",
                  m_downsample_size.width(), m_downsample_size.height(),
                  m_display_scale);

    // =========================================================================
    // Transfer to rendering item
    // =========================================================================

    m_rendering_item->setImage(std::move(source_image));

    // =========================================================================
    // Emit signals
    // =========================================================================

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

    spdlog::debug("[DisplayManager::updateDisplayTile]: Updating display {}x{}",
                  m_display_image_size.width(), m_display_image_size.height());

    m_rendering_item->updateTile(std::move(source_tile));
}

void DisplayManager::setSourceImageSize(int width, int height)
{
    // =========================================================================
    // Store source size
    // =========================================================================

    m_source_image_size = QSize(width, height);

    spdlog::debug("[DisplayManager::setSourceImageSize]: Source image size: {}x{}", width, height);

    // =========================================================================
    // Calculate display parameters via ViewportManager
    // =========================================================================

    if (!isInitialized()) {
        spdlog::warn("[DisplayManager::setSourceImageSize]: Not initialized, using source size as display size");
        m_display_image_size = m_source_image_size;
        m_downsample_size = m_source_image_size;
        m_display_scale = 1.0f;
        m_fit_zoom = 1.0f;
    } else {
        ViewportCalculation calc = m_viewport_manager->calculateDisplay(m_source_image_size);

        if (calc.isValid()) {
            m_display_image_size = calc.display_size;
            m_downsample_size = calc.downsample_size;
            m_fit_zoom = calc.fit_zoom;

            // Calculate display scale
            if (m_source_image_size.width() > 0) {
                m_display_scale = static_cast<float>(m_downsample_size.width()) /
                                  static_cast<float>(m_source_image_size.width());
            }

            spdlog::info("[DisplayManager::setSourceImageSize]: Calculated display:");
            spdlog::info("[DisplayManager::setSourceImageSize]:  Source: {}x{}", m_source_image_size.width(), m_source_image_size.height());
            spdlog::info("[DisplayManager::setSourceImageSize]:  Display: {}x{}", m_display_image_size.width(), m_display_image_size.height());
            spdlog::info("[DisplayManager::setSourceImageSize]:  Downsample: {}x{}", m_downsample_size.width(), m_downsample_size.height());
            spdlog::info("[DisplayManager::setSourceImageSize]:  Fit zoom: {:.3f}", m_fit_zoom);
            spdlog::info("[DisplayManager::setSourceImageSize]:  Texture memory: {} MB", calc.texture_memory_mb);

            // Request new downsampled image if needed
            if (calc.needs_downsample) {
                spdlog::debug("[DisplayManager::setSourceImageSize]: Requesting downsampled image: {}x{}",
                              m_downsample_size.width(), m_downsample_size.height());
                emit displayImageRequest(m_downsample_size.width(), m_downsample_size.height());
            }
        }
    }

    // =========================================================================
    // Emit signals
    // =========================================================================

    emit sourceImageSizeChanged(m_source_image_size);
    emit displayImageSizeChanged(m_display_image_size);
    emit downsampleSizeChanged(m_downsample_size);
    emit displayScaleChanged(m_display_scale);
}

void DisplayManager::setZoom(float zoom)
{
    // =========================================================================
    // Clamp to valid range
    // =========================================================================

    const float clamped_zoom { std::clamp(zoom, 0.1f, 10.0f) };

    if (std::abs(m_zoom - clamped_zoom) < 0.001f) {
        return;
    }

    spdlog::debug("[DisplayManager::setZoom]: Zoom: {:.3f} → {:.3f}", m_zoom, clamped_zoom);

    m_zoom = clamped_zoom;
    constrainPan();

    // Apply to rendering item
    if (m_rendering_item) {
        m_rendering_item->setZoom(m_zoom);
    }

    emit zoomChanged(m_zoom);
}

void DisplayManager::setPan(const QPointF& pan)
{
    if (m_pan == pan) {
        spdlog::trace("[DisplayManager::setPan]: Pan is already set to ({:.3f}, {:.3f}), skipping update", pan.x(), pan.y());
        return;
    }

    m_pan = pan;
    constrainPan();

    if (m_rendering_item) {
        m_rendering_item->setPan(m_pan);
    }

    emit panChanged(m_pan);
}

void DisplayManager::zoomAt(const QPointF& point, float zoom_delta)
{
    // =========================================================================
    // Calculate new zoom
    // =========================================================================

    float old_zoom { m_zoom };
    float new_zoom { std::clamp(old_zoom * zoom_delta, 0.1f, 10.0f) };

    // =========================================================================
    // Adjust pan to keep point under cursor
    // =========================================================================

    const QPointF adjusted_pan { point - (point - m_pan) * (new_zoom / old_zoom) };

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

    spdlog::debug("[DisplayManager::fitToView]: m_display_image_size={}x{}, m_source_image_size={}x{}",
                  m_display_image_size.width(), m_display_image_size.height(),
                  m_source_image_size.width(), m_source_image_size.height());

    spdlog::debug("[DisplayManager::fitToView]: viewport={}x{}",
                  m_viewport_manager ? m_viewport_manager->viewportSize().width() : -1,
                  m_viewport_manager ? m_viewport_manager->viewportSize().height() : -1);

    bool image_valid { !m_downsample_size.isEmpty() };
    bool viewport_manager_valid { m_viewport_manager != nullptr };
    bool viewport_valid { m_viewport_manager && !m_viewport_manager->viewportSize().isEmpty() };

    spdlog::debug("[DisplayManager::fitToView]: image_valid={}, viewport_manager_valid={}, viewport_valid={}",
                  image_valid, viewport_manager_valid, viewport_valid);

    if (!image_valid || !viewport_manager_valid || !viewport_valid) {
        spdlog::warn("[DisplayManager::fitToView]: Image size or viewport is empty, using defaults");
        m_zoom = 1.0f;
        m_pan = QPointF(0.0, 0.0);
    } else {
        const QSize viewport { m_viewport_manager->viewportSize() };

        // ============================================================
        // Calculate zoom to fit downsampled image in viewport
        // ============================================================
        const float zoom_x = static_cast<float>(viewport.width()) / m_downsample_size.width();
        const float zoom_y = static_cast<float>(viewport.height()) / m_downsample_size.height();
        m_zoom = std::min(zoom_x, zoom_y);

        // ============================================================
        // Calculate pan to center the image
        // ============================================================
        const float image_displayed_width = m_downsample_size.width() * m_zoom;
        const float image_displayed_height = m_downsample_size.height() * m_zoom;
        
        const float pan_x = (viewport.width() - image_displayed_width) / 2.0f;
        const float pan_y = (viewport.height() - image_displayed_height) / 2.0f;
        m_pan = QPointF(pan_x, pan_y);

        spdlog::debug("[DisplayManager::fitToView]: zoom={:.3f}, pan=({:.1f}, {:.1f})",
                      m_zoom, m_pan.x(), m_pan.y());
    }

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


void DisplayManager::zoomIn()
{
    setZoom(m_zoom * 1.2f);
}

void DisplayManager::zoomOut()
{
    setZoom(m_zoom / 1.2f);
}


// =============================================================================
// Viewport
// =============================================================================

void DisplayManager::setViewportSize(const QSize& size) {
    if (!size.isValid() || size.isEmpty()) {
        spdlog::warn("[DisplayManager::setViewportSize]: Invalid viewport size");
        return;
    }

    if (!m_viewport_manager) {
        spdlog::error("[DisplayManager::setViewportSize]: ViewportManager not created");
        return;
    }


    const QSize old_viewport { m_viewport_manager->viewportSize() };
    const int old_max { m_viewport_manager->maxDownsample() };

    m_viewport_manager->setViewportSize(size);

    // Check if max downsample changed
    const int new_max { m_viewport_manager->maxDownsample() };
    if (new_max != old_max) {
        spdlog::debug("[DisplayManager::setViewportSize]: Max downsample changed: {} → {}", old_max, new_max);
        emit maxDownsampleChanged(new_max);

        // Recalculate display if we have a source image
        if (m_source_image_size.isValid()) {
            ViewportCalculation calc = m_viewport_manager->calculateDisplay(m_source_image_size);

            if (calc.isValid() && calc.downsample_size != m_downsample_size) {
                m_display_image_size = calc.display_size;
                m_downsample_size = calc.downsample_size;
                m_fit_zoom = calc.fit_zoom;

                emit displayImageSizeChanged(m_display_image_size);
                emit downsampleSizeChanged(m_downsample_size);
                emit displayImageRequest(m_downsample_size.width(), m_downsample_size.height());
            }
        }
    }

    if (old_viewport != size) {
        emit viewportSizeChanged(size);
    }
}

void DisplayManager::setQualityMargin(float margin)
{
    if (!m_viewport_manager) {
        spdlog::error("[DisplayManager::setQualityMargin]: ViewportManager not created");
        return;
    }

    const int old_max { m_viewport_manager->maxDownsample() };
    m_viewport_manager->setQualityMargin(margin);
    const int new_max { m_viewport_manager->maxDownsample() };

    if (new_max != old_max) {
        emit maxDownsampleChanged(new_max);
    }
}
// =============================================================================
// Coordinate Mapping
// =============================================================================
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


// =============================================================================
// Internal Methods
// =============================================================================


void DisplayManager::constrainPan()
{
    if (m_display_image_size.isEmpty()) {
        spdlog::trace("[DisplayManager::constrainPan]: Display image size is empty, skipping constraint");
        return;
    }

    // Get viewport size
    const QSize viewport { m_viewport_manager ? m_viewport_manager->viewportSize() : QSize(800, 600) };

    // Calculate visible area at current zoom
    const float visible_width { m_display_image_size.width() * m_zoom };
    const float visible_height { m_display_image_size.height() * m_zoom };

    // Calculate maximum pan in each direction
    const float max_pan_x { std::max(0.0f, (visible_width - viewport.width()) / 2.0f) };
    const float max_pan_y { std::max(0.0f, (visible_height - viewport.height()) / 2.0f) };

    // Clamp pan
    const QPointF old_pan = m_pan;
    m_pan.setX(std::clamp(static_cast<float>(m_pan.x()), -max_pan_x, max_pan_x));
    m_pan.setY(std::clamp(static_cast<float>(m_pan.y()), -max_pan_y, max_pan_y));

    if (m_pan != old_pan) {
        spdlog::trace("[DisplayManager::constrainPan]: Pan constrained: ({:.1f}, {:.1f}) → ({:.1f}, {:.1f})",
                      old_pan.x(), old_pan.y(), m_pan.x(), m_pan.y());
    }
}

QSize DisplayManager::calculateDisplaySize(const QSize& source_size) const
{
    if (!m_viewport_manager || !source_size.isValid()) {
        return {};
    }

    ViewportCalculation calc = m_viewport_manager->calculateDisplay(source_size);
    return calc.display_size;
}

// =============================================================================
// Property Getters
// =============================================================================

int DisplayManager::maxDownsample() const noexcept
{
    return m_viewport_manager ? m_viewport_manager->maxDownsample() : 2048;
}

QSize DisplayManager::viewportSize() const noexcept
{
    return m_viewport_manager ? m_viewport_manager->viewportSize() : QSize();
}

} // namespace CaptureMoment::UI::Display
