// --- qml_painted_image_item.h ---
#pragma once

#include "rendering/painted_image_item.h" 

namespace CaptureMoment::UI {

class QMLPaintedImageItem : public Rendering::PaintedImageItem {
Q_OBJECT
 
Q_PROPERTY(float zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
Q_PROPERTY(QPointF pan READ pan WRITE setPan NOTIFY panChanged)
Q_PROPERTY(int imageWidth READ imageWidth NOTIFY imageDimensionsChanged)
Q_PROPERTY(int imageHeight READ imageHeight NOTIFY imageDimensionsChanged)

public:
    explicit QMLPaintedImageItem(QQuickItem* parent = nullptr);
    ~QMLPaintedImageItem();
};

} // namespace CaptureMoment::UI