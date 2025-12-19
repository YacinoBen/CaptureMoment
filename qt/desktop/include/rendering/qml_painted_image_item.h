/**
 * @file qml_painted_image_item.h
 * @brief QML-compatible wrapper for PaintedImageItem.
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <QQuickItem>
#include "rendering/painted_image_item.h"

namespace CaptureMoment::UI {

/**
 * @brief QQuickItem wrapping PaintedImageItem for direct use in QML.
 *
 * This class inherits from PaintedImageItem, which itself inherits from QQuickItem.
 * It re-exposes (or potentially adds) QML-specific properties (zoom, pan, image dimensions)
 * defined in its parent classes (PaintedImageItem, IRenderingItemBase) to make them
 * readily available for binding in QML files.
 * This class acts as a bridge to ensure seamless integration of the painted item's
 * capabilities (managed by the core Qt Quick scene graph and QPainter) with QML.
 */
class QMLPaintedImageItem : public Rendering::PaintedImageItem {
Q_OBJECT

    // Expose properties from PaintedImageItem (and potentially IRenderingItemBase) to QML
    // These properties map directly to getter/setter methods and signals defined in PaintedImageItem.
    Q_PROPERTY(float zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    Q_PROPERTY(QPointF pan READ pan WRITE setPan NOTIFY panChanged)
    Q_PROPERTY(int imageWidth READ imageWidth NOTIFY imageDimensionsChanged)
    Q_PROPERTY(int imageHeight READ imageHeight NOTIFY imageDimensionsChanged)

public:
    /**
     * @brief Constructs a new QMLPaintedImageItem.
     * @param parent Optional parent QQuickItem.
     */
    explicit QMLPaintedImageItem(QQuickItem* parent = nullptr);

    /**
     * @brief Destroys the QMLPaintedImageItem.
     * 
     * The destructor of the parent class (PaintedImageItem) will be called automatically,
     * cleaning up its resources.
     */
    ~QMLPaintedImageItem();
    };

} // namespace CaptureMoment::UI
