/**
 * @file base_image_item.h
 * @brief Base class for Qt Quick image display items managing common state (zoom, pan, dimensions) and QML properties/signals.
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <QQuickItem>
#include <QPointF> // For QPointF
#include <QMutex>  // If needed in derived classes
#include <QSize>   // If needed
#include "rendering/i_rendering_item_base.h"

namespace CaptureMoment::UI::Rendering {

/**
 * @brief Base class for Qt Quick image display items managing common state (zoom, pan, dimensions) and QML properties/signals.
 *
 * This class inherits from QQuickItem and defines common QML properties (zoom, pan, image dimensions)
 * and their associated signals (zoomChanged, panChanged, imageSizeChanged).
 * It also provides basic member variables for zoom, pan, and image dimensions.
 * It does NOT implement the IRenderingItemBase interface (that's done by derived classes).
 * It provides a common Qt Quick foundation for different rendering implementations (SGS, Painted, RHI).
 * Note: m_image_width and m_image_height are protected by m_image_mutex and require locking for thread-safe access.
 * m_zoom and m_pan are expected to be modified only on the main thread.
 */
class BaseImageItem : public QQuickItem,  public IRenderingItemBase // Note: IRenderingItemBase n'a pas de Q_OBJECT
{
    Q_OBJECT

    // Expose common properties to QML
    Q_PROPERTY(float zoom READ zoom NOTIFY zoomChanged)
    Q_PROPERTY(QPointF pan READ pan NOTIFY panChanged)
    Q_PROPERTY(int imageWidth READ imageWidth NOTIFY imageSizeChanged)
    Q_PROPERTY(int imageHeight READ imageHeight NOTIFY imageSizeChanged)

protected:
    /**
     * @brief Current zoom level applied to the image.
     * A value of 1.0f represents the original size.
     * Expected to be modified only on the main thread.
     */
    float m_zoom{1.0f};

    /**
     * @brief Current pan offset applied to the image.
     * Represents the offset in scene coordinates.
     * Expected to be modified only on the main thread.
     */
    QPointF m_pan{0, 0};

    // Image metadata (these will be managed by derived classes)
    /**
     * @brief Width of the currently loaded image in pixels.
     * This member is typically managed by derived classes and protected by m_image_mutex.
     */
    int m_image_width{0};

    /**
     * @brief Height of the currently loaded image in pixels.
     * This member is typically managed by derived classes and protected by m_image_mutex.
     */
    int m_image_height{0};

public:
    /**
     * @brief Constructs a BaseImageItem.
     * @param parent Optional parent QQuickItem.
     */
    explicit BaseImageItem(QQuickItem* parent = nullptr);

    /**
     * @brief Gets the current zoom level.
     * This value is expected to be modified only on the main thread, no mutex needed here.
     * @return The current zoom factor.
     */
    [[nodiscard]] float zoom() const { return m_zoom; }

    /**
     * @brief Gets the current pan offset.
     * This value is expected to be modified only on the main thread, no mutex needed here.
     * @return The current pan offset.
     */
    [[nodiscard]] QPointF pan() const { return m_pan; }

    /**
     * @brief Gets the width of the image.
     * This method provides thread-safe access to m_image_width.
     * @return The image width in pixels.
     */
    [[nodiscard]] virtual int imageWidth() const override; // Doit implémenter le verrouillage

    /**
     * @brief Gets the height of the image.
     * This method provides thread-safe access to m_image_height.
     * @return The image height in pixels.
     */
    [[nodiscard]] virtual int imageHeight() const override; // Doit implémenter le verrouillage

    /**
     * @brief Sets the zoom level.
     * This method should be called only from the main thread.
     * This method should be overridden by derived classes to update m_zoom
     * and potentially trigger a repaint (update()).
     * @param zoom The new zoom factor (e.g., 1.0f for original size).
     */
    virtual void setZoom(float zoom) override; // Doit implémenter update() si nécessaire

    /**
     * @brief Sets the pan offset.
     * This method should be called only from the main thread.
     * This method should be overridden by derived classes to update m_pan
     * and potentially trigger a repaint (update()).
     * @param pan The new pan offset as a QPointF.
     */
    virtual void setPan(const QPointF& pan) override; // Doit implémenter update() si nécessaire

signals:
    /**
     * @brief Signal emitted when the zoom value changes.
     * @param zoom The new zoom factor.
     */
    void zoomChanged(float zoom);

    /**
     * @brief Signal emitted when the pan offset changes.
     * @param pan The new pan offset.
     */
    void panChanged(const QPointF& pan);

    /**
     * @brief Signal emitted when the image dimensions change (width or height).
     */
    void imageSizeChanged();
};

} // namespace CaptureMoment::UI::Rendering
