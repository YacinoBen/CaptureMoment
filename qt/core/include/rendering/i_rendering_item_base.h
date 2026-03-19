/**
 * @file rendering_item_base.h
 * @brief Abstract base class for image display items managing common state (zoom, pan, dimensions).
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <QObject>
#include <QMutex>
#include <QPointF>
#include <memory>

#include "common/image_region.h"

namespace CaptureMoment::UI {

/**
 * @brief Namespace containing Qt-specific UI components for CaptureMoment.
 * 
 * This namespace includes classes responsible for rendering and UI integration
 * using Qt Quick and potentially the Qt Rendering Hardware Interface (QRhi).
 */
namespace Rendering {

/**
 * @brief Abstract base class for image display items managing common state (zoom, pan, dimensions).
 *
 * This class provides a common interface and shared state management for image display items
 * zoom, pan, image dimensions, and the basic setImage/updateTile methods.
 * It does not inherit from QQuickItem or any other Qt Quick base class, allowing
 */
class IRenderingItemBase {

public:
    /**
     * @brief Virtual destructor for safe inheritance.
     */
    virtual ~IRenderingItemBase() = default;
    
    /**
     * @brief Sets the full image to be displayed.
     * 
     * This method safely updates the internal image data and marks the display
     * for an update on the next render pass.
     * 
     * @param image The image data.
     */
    virtual void setImage(std::unique_ptr<Core::Common::ImageRegion> image) = 0;
    
    /**
     * @brief Updates a specific tile of the displayed image.
     * 
     * This method merges the data from the provided tile into the internal image buffer
     * and marks the display for an update. It's intended for incremental updates
     * after processing specific regions.
     * 
     * @param tile The image tile containing the updated region.
     */
    virtual void updateTile(std::unique_ptr<Core::Common::ImageRegion> tile) = 0;
    
    /**
     * @brief Sets the zoom level.
     * @param zoom The new zoom factor (e.g., 1.0f for original size).
     */
    virtual void setZoom(float zoom) = 0;

    /**
     * @brief Sets the pan offset.
     * @param pan The new pan offset as a QPointF.
     */
    virtual void setPan(const QPointF& pan) = 0;

    /**
     * @brief Gets the current zoom level.
     * @return The current zoom factor.
     */
    [[nodiscard]] virtual float zoom() const = 0;

    /**
     * @brief Gets the current pan offset.
     * @return The current pan offset.
     */
    [[nodiscard]] virtual QPointF pan() const = 0;

    /**
     * @brief Get the width of the image.
     * @return The image width in pixels, or 0 if no image is loaded.
     */
    [[nodiscard]] virtual int imageWidth() const = 0;

    /**
     * @brief Get the height of the image.
     * @return The image height in pixels, or 0 if no image is loaded.
     */
    [[nodiscard]] virtual int imageHeight() const = 0;

    /**
     * @brief Checks if the full image data is loaded and valid.
     * @return True if the image is valid, false otherwise.
     */
    [[nodiscard]] virtual bool isImageValid() const = 0;

    /**
     * @brief Gets a pointer to the mutex protecting the image data.
     * This method provides access to the mutex for thread-safe operations.
     * The mutex is mutable, so it can be locked even on a const object.
     * @return A pointer to the image mutex.
     */
    [[nodiscard]] virtual QMutex* getImageMutex() const = 0;

    /**
     * @brief Gets a pointer to the full image data.
     * This method provides access to the internal image data pointer.
     * @return A pointer to the ImageRegion, or nullptr if no image is loaded.
     */
    [[nodiscard]] virtual const Core::Common::ImageRegion* getFullImage() const = 0;
};

} // namespace Rendering

} // namespace CaptureMoment::UI
