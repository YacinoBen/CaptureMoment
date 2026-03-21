/**
 * @file viewport_manager.h
 * @brief Central viewport management for optimal display across all screen types.
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "viewport_config.h"
#include <QObject>
#include <QSize>
#include <QPointF>

namespace CaptureMoment::UI {
namespace Display {

/**
 * @struct ScreenInfo
 * @brief Physical screen characteristics.
 */
struct ScreenInfo {
    int physical_width{0};   ///< Physical screen width in pixels
    int physical_height{0};  ///< Physical screen height in pixels
    float dpr{1.0f};         ///< Device pixel ratio (density)
    
    [[nodiscard]] bool isValid() const noexcept {
        return physical_width > 0 && physical_height > 0 && dpr > 0.0f;
    }
};

/**
 * @struct ViewportCalculation
 * @brief Result of viewport calculation for an image.
 */
struct ViewportCalculation {
    QSize downsample_size;       ///< Size to downsample source image to (GPU texture)
    QSize display_size;          ///< Size to display in QML (logical pixels)
    float fit_zoom{1.0f};        ///< Zoom factor to fit image in viewport (0.0-1.0)
    bool needs_downsample{false};///< True if downsample is required
    size_t texture_memory_mb{0}; ///< Estimated GPU memory usage in MB
    
    [[nodiscard]] bool isValid() const noexcept {
        return downsample_size.isValid() && display_size.isValid();
    }
};

/**
 * @struct ViewportState
 * @brief Current viewport state for zoom/pan operations.
 */
struct ViewportState {
    float zoom{1.0f};            ///< Current zoom level (1.0 = 100%)
    QPointF pan{0.0, 0.0};       ///< Current pan offset in logical pixels
    QPointF center_offset{0.0, 0.0}; ///< Offset to center image in viewport
};

/**
 * @class ViewportManager
 * @brief Manages viewport calculations for optimal display across all screen types.
 * 
 * This class implements screen-based viewport management:
 * - Initialized once with screen and viewport information
 * - Calculates optimal downsample size per image
 * - Never upscales (source size is maximum)
 */
class ViewportManager : public QObject {
    Q_OBJECT

public:
    // =========================================================================
    // Construction
    // =========================================================================
    
    explicit ViewportManager(QObject* parent = nullptr);
    ~ViewportManager() override = default;
    
    ViewportManager(const ViewportManager&) = delete;
    ViewportManager& operator=(const ViewportManager&) = delete;
    ViewportManager(ViewportManager&&) = default;
    ViewportManager& operator=(ViewportManager&&) = default;

    // =========================================================================
    // Initialization
    // =========================================================================
    
    /**
     * @brief Initializes the manager with screen and viewport parameters.
     * 
     * Calculation steps:
     * 1. Store screen info and viewport size
     * 2. Calculate viewport physical size = viewport × DPR
     * 3. Determine plafond based on screen physical size
     * 4. Calculate max downsample = min(viewport_physical × margin, plafond)
     * 
     * @param viewport_logical Logical viewport size in QML coordinates.
     * @param screen Physical screen information.
     */
    void initialize(const QSize& viewport_logical, const ScreenInfo& screen);
    
    /**
     * @brief Updates the viewport size.
     * @param viewport_logical New logical viewport size.
     */
    void setViewportSize(const QSize& viewport_logical);
    
    /**
     * @brief Sets the quality margin for zoom headroom.
     * @param margin Quality margin (clamped to 1.0-2.0).
     */
    void setQualityMargin(float margin);
    
    /**
     * @brief Sets the maximum downsample dimension manually.
     * @param max_dim Maximum dimension in pixels (minimum 256).
     */
    void setMaxDownsample(int max_dim);

    // =========================================================================
    // Calculations
    // =========================================================================
    
    /**
     * @brief Calculates optimal display parameters for a source image.
     * 
     * Algorithm:
     * 1. Calculate fit_zoom = min(viewport/source_w, viewport/source_h, 1.0)
     * 2. Calculate display_size = source × fit_zoom
     * 3. Calculate downsample_size = min(source, max_downsample)
     * 4. Determine needs_downsample flag
     * 5. Estimate GPU memory usage
     * 
     * @param source_size Source image dimensions in pixels.
     * @return ViewportCalculation with all display parameters.
     */
    [[nodiscard]] ViewportCalculation calculateDisplay(const QSize& source_size) const;
    
    /**
     * @brief Calculates the zoom factor to fit an image in the viewport.
     * 
     * Always ≤ 1.0 (never upscales).
     * Formula: fit_zoom = min(viewport_w/source_w, viewport_h/source_h, 1.0)
     * 
     * @param source_size Source image dimensions.
     * @return Zoom factor (0.0-1.0), or 1.0 if invalid input.
     */
    [[nodiscard]] float calculateFitZoom(const QSize& source_size) const noexcept;
    
    /**
     * @brief Calculates the downsample size for a source image.
     * 
     * Applies the golden rule: **NEVER UPSCALE**
     * 
     * @param source_size Source image dimensions.
     * @return Optimal downsample size, or invalid QSize if input invalid.
     */
    [[nodiscard]] QSize calculateDownsampleSize(const QSize& source_size) const;
    
    /**
     * @brief Calculates viewport state for fit-to-view.
     * 
     * @param display_size The display image size in logical pixels.
     * @return ViewportState with centered position and zoom=1.0.
     */
    [[nodiscard]] ViewportState calculateFitToView(const QSize& display_size) const noexcept;

    // =========================================================================
    // Getters
    // =========================================================================
    
    /**
     * @brief Gets the logical viewport size.
     * @return Logical viewport size in QML coordinates.
     */
    [[nodiscard]] QSize viewportSize() const noexcept { return m_viewport_logical; }

    /**
     * @brief Gets the physical viewport size.
     * @return Physical viewport size in pixels (viewport × DPR).
     */
    [[nodiscard]] QSize viewportPhysicalSize() const noexcept { return m_viewport_physical; }

    /**
     * @brief Gets the screen information.
     * @return Screen information (size, DPR).
     */
    [[nodiscard]] ScreenInfo screenInfo() const noexcept { return m_screen; }
  
    /**
     * @brief Gets the device pixel ratio.
     * @return Device pixel ratio (DPR).
     */
    [[nodiscard]] float devicePixelRatio() const noexcept { return m_screen.dpr; }
    
    /**
     * @brief Gets the current quality margin.
     * @return Quality margin for zoom headroom (1.0-2.0).
     */
    [[nodiscard]] float qualityMargin() const noexcept { return m_quality_margin; }
  
    /**
     * @brief Gets the screen-based ceiling.
     * @return Screen-based ceiling (2K or 3K).
     */
    [[nodiscard]] int plafond() const noexcept { return m_plafond; }
    
    /**
     * @brief Gets the maximum downsample dimension.
     * @return Maximum downsample dimension (calculated from viewport and plafond).
     */
    [[nodiscard]] int maxDownsample() const noexcept { return m_max_downsample; }
    
    /**
     * @brief Gets the initialization status.
     * @return True if the viewport manager is initialized, false otherwise.
     */
    [[nodiscard]] bool isInitialized() const noexcept { return m_initialized; }

signals:
    void viewportSizeChanged(const QSize& size);
    void maxDownsampleChanged(int max_dim);

private:

    /**
     * @brief Updates the maximum downsample dimension based on current viewport and plafond.
     */
    void updateMaxDownsample();

    /**
     * @brief Whether initialize() has been called
     */
    bool m_initialized{false}; 

    /**
     * @brief Logical viewport size in QML coordinates
     */
    QSize m_viewport_logical;   

    /**
     * @brief Physical viewport size in pixels (viewport × DPR)
     */
    QSize m_viewport_physical;

    /**
     * @brief Physical screen information (size, DPR)
     */
    ScreenInfo m_screen;

    /**
     * @brief Quality margin for zoom headroom (1.0-2.0)
     */
    float m_quality_margin{DisplayConfig::defaultQualityMargin()}; 

    /**
    * @brief Screen-based ceiling (2K or 3K)
    */
    int m_plafond{DisplayConfig::plafondSmallScreen()};

    /**
     * @brief Maximum downsample dimension (calculated from viewport and plafond)
     */
    int m_max_downsample{DisplayConfig::plafondSmallScreen()};
};

} // namespace Display
} // namespace CaptureMoment::UI
