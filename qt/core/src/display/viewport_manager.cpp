/**
 * @file viewport_manager.cpp
 * @brief Implementation of ViewportManager.
 * @author CaptureMoment Team
 * @date 2026
 */

#include "display/viewport_manager.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>

namespace CaptureMoment::UI::Display {

// =============================================================================
// Construction
// =============================================================================

ViewportManager::ViewportManager(QObject* parent)
    : QObject(parent)
{
    spdlog::debug("[ViewportManager::ViewportManager]: Created");
}

// =============================================================================
// Initialization
// =============================================================================

void ViewportManager::initialize(const QSize& viewport_logical, const ScreenInfo& screen)
{
    // =========================================================================
    // Step 1: Validate inputs
    // =========================================================================
    
    if (!viewport_logical.isValid() || viewport_logical.isEmpty()) {
        spdlog::error("[ViewportManager::initialize]: Invalid viewport size");
        return;
    }
    
    if (!screen.isValid()) {
        spdlog::error("[ViewportManager::initialize]: Invalid screen info");
        return;
    }
    
    // =========================================================================
    // Step 2: Store parameters
    // =========================================================================
    
    m_viewport_logical = viewport_logical;
    m_screen = screen;
    
    // =========================================================================
    // Step 3: Calculate viewport physical size
    // =========================================================================
    // Physical size = logical size × DPR
    // This is the actual number of pixels Qt will render
    
    m_viewport_physical = QSize(
        static_cast<int>(std::round(viewport_logical.width() * screen.dpr)),
        static_cast<int>(std::round(viewport_logical.height() * screen.dpr))
    );
    
    // =========================================================================
    // Step 4: Determine plafond based on screen physical size
    // =========================================================================
    // Screens >= 2000px use 3K plafond
    // Screens < 2000px use 2K plafond
    
    m_plafond = determinePlafond(screen.physical_width, screen.physical_height);
    
    // =========================================================================
    // Step 5: Calculate maximum downsample dimension
    // =========================================================================
    
    updateMaxDownsample();
    
    // =========================================================================
    // Step 6: Mark as initialized
    // =========================================================================
    
    m_initialized = true;
    
    // =========================================================================
    // Step 7: Log configuration
    // =========================================================================
    
    spdlog::info("[ViewportManager::initialize]: Initialized:");
    spdlog::info("[ViewportManager::initialize]:  Screen: {}x{}, DPR: {:.2f}",
                 m_screen.physical_width, m_screen.physical_height, m_screen.dpr);
    spdlog::info("[ViewportManager::initialize]:  Viewport: {}x{} (physical: {}x{})",
                 m_viewport_logical.width(), m_viewport_logical.height(),
                 m_viewport_physical.width(), m_viewport_physical.height());
    spdlog::info("[ViewportManager::initialize]:  Plafond: {}px, Max downsample: {}px",
                 m_plafond, m_max_downsample);
    
    emit viewportSizeChanged(m_viewport_logical);
}

void ViewportManager::setViewportSize(const QSize& viewport_logical)
{
    if (!viewport_logical.isValid() || viewport_logical.isEmpty()) {
        spdlog::warn("[ViewportManager::setViewportSize]: Invalid viewport size");
        return;
    }
    
    if (m_viewport_logical == viewport_logical) {
        return;
    }
    
    m_viewport_logical = viewport_logical;
    m_viewport_physical = QSize(
        static_cast<int>(std::round(viewport_logical.width() * m_screen.dpr)),
        static_cast<int>(std::round(viewport_logical.height() * m_screen.dpr))
    );
    
    updateMaxDownsample();
    
    spdlog::debug("[ViewportManager::setViewportSize]: Viewport resized: {}x{}",
                  m_viewport_logical.width(), m_viewport_logical.height());
    
    emit viewportSizeChanged(m_viewport_logical);
}

void ViewportManager::setQualityMargin(float margin)
{
    m_quality_margin = std::clamp(margin, DisplayConfig::minQualityMargin(), DisplayConfig::maxQualityMargin());
    
    if (m_initialized) {
        updateMaxDownsample();
    }
    
    spdlog::debug("[ViewportManager::setQualityMargin]: Quality margin: {:.2f}", m_quality_margin);
}

void ViewportManager::setMaxDownsample(int max_dim)
{
    m_max_downsample = std::max(max_dim, DisplayConfig::minDisplayDimension());
    spdlog::info("[ViewportManager::setMaxDownsample]: Max downsample set: {}px", m_max_downsample);
    emit maxDownsampleChanged(m_max_downsample);
}

// =============================================================================
// Calculations
// =============================================================================

ViewportCalculation ViewportManager::calculateDisplay(const QSize& source_size) const
{
    ViewportCalculation result;
    
    // =========================================================================
    // Step 1: Validate input
    // =========================================================================
    
    if (!source_size.isValid() || source_size.isEmpty()) {
        spdlog::warn("[ViewportManager::calculateDisplay]: Invalid source size");
        return result;
    }
    
    if (!m_initialized) {
        spdlog::warn("[ViewportManager::calculateDisplay]: Not initialized");
        return result;
    }
    
    const int src_w = source_size.width();
    const int src_h = source_size.height();
    
    // =========================================================================
    // Step 2: Calculate fit-to-view zoom
    // =========================================================================
    // This is the zoom that makes the entire image visible in viewport
    // Always <= 1.0 (never upscale)
    
    result.fit_zoom = calculateFitZoom(source_size);
    
    // =========================================================================
    // Step 3: Calculate logical display size
    // =========================================================================
    // This is what the user sees in QML coordinates
    
    result.display_size = QSize(
        static_cast<int>(std::round(src_w * result.fit_zoom)),
        static_cast<int>(std::round(src_h * result.fit_zoom))
    );
    
    // =========================================================================
    // Step 4: Calculate downsample size
    // =========================================================================
    // Apply golden rule: NEVER UPSCALE
    // downsample_size = min(source_size, max_downsample)
    
    result.downsample_size = calculateDownsampleSize(source_size);
    
    // =========================================================================
    // Step 5: Determine if downsample is needed
    // =========================================================================
    
    result.needs_downsample = (result.downsample_size != source_size);
    
    // =========================================================================
    // Step 6: Calculate GPU memory usage
    // =========================================================================
    
    result.texture_memory_mb = calculateTextureMemoryMB(result.downsample_size);
    
    // =========================================================================
    // Step 7: Log result (debug only)
    // =========================================================================
    
    spdlog::debug("[ViewportManager::calculateDisplay]: Display calc: source={}x{}, display={}x{}, downsample={}x{}, fit_zoom={:.3f}",
                  src_w, src_h,
                  result.display_size.width(), result.display_size.height(),
                  result.downsample_size.width(), result.downsample_size.height(),
                  result.fit_zoom);
    
    return result;
}

float ViewportManager::calculateFitZoom(const QSize& source_size) const noexcept
{
    // =========================================================================
    // Validate inputs
    // =========================================================================
    
    if (!source_size.isValid() || source_size.isEmpty()) {
        return 1.0f;
    }
    
    if (!m_viewport_logical.isValid() || m_viewport_logical.isEmpty()) {
        return 1.0f;
    }
    
    // =========================================================================
    // Calculate scale factors for both dimensions
    // =========================================================================
    
    const float scale_x = static_cast<float>(m_viewport_logical.width()) / 
                          static_cast<float>(source_size.width());
    const float scale_y = static_cast<float>(m_viewport_logical.height()) / 
                          static_cast<float>(source_size.height());
    
    // =========================================================================
    // Use the smaller scale to ensure image fits entirely
    // =========================================================================
    
    float fit_zoom = std::min(scale_x, scale_y);
    
    // =========================================================================
    // Never upscale beyond 100%
    // =========================================================================
    
    return std::min(fit_zoom, 1.0f);
}

QSize ViewportManager::calculateDownsampleSize(const QSize& source_size) const
{
    // =========================================================================
    // Validate input
    // =========================================================================
    
    if (!source_size.isValid() || source_size.isEmpty()) {
        return {};
    }
    
    const int src_w = source_size.width();
    const int src_h = source_size.height();
    
    // =========================================================================
    // GOLDEN RULE: Never upscale
    // =========================================================================
    // If source is smaller than max, return source unchanged
    
    if (src_w <= m_max_downsample && src_h <= m_max_downsample) {
        spdlog::trace("[ViewportManager::calculateDownsampleSize]: No downsample needed (source {}x{} <= max {})",
                      src_w, src_h, m_max_downsample);
        return source_size;
    }
    
    // =========================================================================
    // Calculate proportional scale
    // =========================================================================
    // Scale the larger dimension to max_downsample
    
    const float scale = static_cast<float>(m_max_downsample) / 
                        static_cast<float>(std::max(src_w, src_h));
    
    const int down_w = static_cast<int>(std::round(src_w * scale));
    const int down_h = static_cast<int>(std::round(src_h * scale));
    
    return QSize(down_w, down_h);
}

ViewportState ViewportManager::calculateFitToView(const QSize& display_size) const noexcept
{
    ViewportState state;
    
    // =========================================================================
    // Validate inputs
    // =========================================================================
    
    if (!display_size.isValid() || !m_viewport_logical.isValid()) {
        return state;
    }
    
    // =========================================================================
    // Set zoom to 100% (fit-to-view means no zoom)
    // =========================================================================
    
    state.zoom = 1.0f;
    
    // =========================================================================
    // Calculate pan offset to center image in viewport
    // =========================================================================
    
    state.center_offset = QPointF(
        (m_viewport_logical.width() - display_size.width()) / 2.0f,
        (m_viewport_logical.height() - display_size.height()) / 2.0f
    );
    
    state.pan = state.center_offset;
    
    return state;
}

// =============================================================================
// Internal Methods
// =============================================================================

void ViewportManager::updateMaxDownsample()
{
    // =========================================================================
    // Calculate max from viewport physical size with quality margin
    // =========================================================================
    
    const int viewport_max = std::max(m_viewport_physical.width(), 
                                      m_viewport_physical.height());
    const int with_margin = static_cast<int>(std::round(viewport_max * m_quality_margin));
    
    // =========================================================================
    // Apply plafond (screen-based ceiling)
    // =========================================================================
    // max_downsample = min(viewport_physical × margin, plafond)
    
    m_max_downsample = std::min(with_margin, m_plafond);
    
    // =========================================================================
    // Ensure minimum size
    // =========================================================================
    
    m_max_downsample = std::max(m_max_downsample, DisplayConfig::minDisplayDimension());
    
    spdlog::debug("[ViewportManager::updateMaxDownsample]: Max downsample: {}px (viewport={}, plafond={})",
                  m_max_downsample, with_margin, m_plafond);
    
    emit maxDownsampleChanged(m_max_downsample);
}

} // namespace CaptureMoment::UI::Display
