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
     * @brief Gets the width of the image.
     * This method provides thread-safe access to m_image_width.
     * @return The image width in pixels.
     */
    [[nodiscard]] virtual int imageWidth() const override;

    /**
     * @brief Gets the height of the image.
     * This method provides thread-safe access to m_image_height.
     * @return The image height in pixels.
     */
    [[nodiscard]] virtual int imageHeight() const override;

    /**
     * @brief Checks if the full image data is loaded and valid.
     * This method provides thread-safe access to check the image state.
     * @return True if the image is valid, false otherwise.
     */
    [[nodiscard]] bool isImageValid() const;
};

} // namespace Rendering

} // namespace CaptureMoment::UI
