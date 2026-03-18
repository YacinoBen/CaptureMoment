/**
 * @file base_image_item.h
 * @brief Base class for Qt Quick image display items managing common state (zoom, pan, dimensions)
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <QQuickItem>
#include <QPointF>
#include <QMutex>
#include <QSize>
#include "rendering/i_rendering_item_base.h"

namespace CaptureMoment::UI {

namespace Rendering {

/**
 * @brief Base class for Qt Quick image display items managing common state dimensions
 *
 * It does NOT implement the IRenderingItemBase interface (that's done by derived classes).
 * It provides a common Qt Quick foundation for different rendering implementations (SGS, Painted, RHI).
 * Note: m_image_width and m_image_height are protected by m_image_mutex and require locking for thread-safe access.
 */
class BaseImageItem : public IRenderingItemBase
{
public:
    /**
     * @brief Constructs a BaseImageItem.
     * @param parent Optional parent QQuickItem.
     */
    explicit BaseImageItem();

    /**
     * @brief Sets the zoom level.
     * @param zoom The new zoom factor (e.g., 1.0f for original size).
     */
    void setZoom(float zoom) override;

    /**
     * @brief Sets the pan offset.
     * @param pan The new pan offset as a QPointF.
     */
    void setPan(const QPointF& pan) override;

    /**
     * @brief Gets the current pan offset.
     * @return The current pan offset.
     */
    [[nodiscard]] QPointF pan() const override { return m_pan; };

    /**
     * @brief Gets the current zoom level.
     * @return The current zoom factor.
     */
    [[nodiscard]] float zoom() const override { return m_zoom; };

        /**
     * @brief Gets the width of the image.
     * This method provides thread-safe access to m_image_width.
     * @return The image width in pixels.
     */
    [[nodiscard]] int imageWidth() const override;

    /**
     * @brief Gets the height of the image.
     * This method provides thread-safe access to m_image_height.
     * @return The image height in pixels.
     */
    [[nodiscard]] int imageHeight() const override;

    /**
     * @brief Checks if the full image data is loaded and valid.
     * This method provides thread-safe access to check the image state.
     * @return True if the image is valid, false otherwise.
     */
    [[nodiscard]] bool isImageValid() const override;


    /**
     * @brief Gets a pointer to the mutex protecting the image data.
     * This method provides access to the mutex for thread-safe operations.
     * The mutex is mutable, so it can be locked even on a const object.
     * @return A pointer to the image mutex.
     */
    [[nodiscard]] QMutex* getImageMutex() const override { return &m_image_mutex; };

    /**
     * @brief Gets a pointer to the full image data.
     * This method provides access to the internal image data pointer.
     * @return A pointer to the ImageRegion, or nullptr if no image is loaded.
     */
    [[nodiscard]] const Core::Common::ImageRegion* getFullImage() const override {
        return m_full_image.get();}

protected:

    /* --------------------- Protected Members --------------------*/

    /**
     * @brief Unique pointer to the full image data displayed by this item.
     *
     * This member holds the complete image data (CPU side, float32) which is used to update the display.
     */
    std::unique_ptr<Core::Common::ImageRegion> m_full_image;

    /**
     * @brief Mutex protecting access to m_full_image and related state.
     * Ensures thread-safe updates to the image data.
     */
    mutable QMutex m_image_mutex;

    /**
     * @brief Current zoom level applied to the image.
     *
     * A value of 1.0f represents the original size.
     */
    float m_zoom{1.0f};

    /**
     * @brief Current pan offset applied to the image.
     *
     * Represents the offset in scene coordinates.
     */
    QPointF m_pan{0, 0};

    /* --------------------- Protected Methods --------------------*/
    /*
     * @brief Virtual methods for handling state changes (can be overridden by derived classes).
     * These methods are called when the corresponding state changes (zoom, pan, image).
     * The base implementation does nothing, but derived classes can override to react to changes.
     */
    virtual void onZoomChanged(float new_zoom) {};

    /*
     * @brief Virtual method for handling pan changes.
     * This method is called when the pan offset changes.
     * The base implementation does nothing, but derived classes can override to react to pan changes.
     */
    virtual void onPanChanged(const QPointF& new_pan) {};

    /*
     * @brief Virtual method for handling image changes.
     * This method is called when the image data changes (setImage or updateTile).
     * The base implementation does nothing, but derived classes can override to react to image changes.
     */
    virtual void onImageChanged() {};
};

} // namespace Rendering

} // namespace CaptureMoment::UI
