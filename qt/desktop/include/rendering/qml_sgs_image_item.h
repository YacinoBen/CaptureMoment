/**
 * @file qml_sgs_image_item.h
 * @brief QML object wrapper for SGSImageItem using modern C++20/23 features.
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <QQuickItem>
#include "rendering/sgs_image_item.h" // Nécessite que SGSImageItem soit inclus
#include <QSize> // Pour QSize si nécessaire
#include <QPointF> // Pour QPointF
#include <QRectF> // Pour QRectF si nécessaire
#include <concepts> // C++20: Pour les concepts (optionnel ici, mais potentiellement utile)

namespace CaptureMoment::UI {

/**
 * @brief QQuickItem for displaying images using QSGSimpleTextureNode, directly usable in QML.
 *
 * This class inherits from SGSImageItem, exposing its properties (zoom, pan, image dimensions)
 * directly as QML properties. It simplifies the QML-side usage compared to
 * having a separate controller and linking them manually.
 * This class is intended to be instantiated directly in QML.
 */
class QMLSGSImageItem : public Rendering::SGSImageItem
{
    Q_OBJECT

    // Expose properties from SGSImageItem to QML
    Q_PROPERTY(float zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    Q_PROPERTY(QPointF pan READ pan WRITE setPan NOTIFY panChanged)
    Q_PROPERTY(int imageWidth READ imageWidth NOTIFY imageSizeChanged) // Utilise imageSizeChanged de SGSImageItem
    Q_PROPERTY(int imageHeight READ imageHeight NOTIFY imageSizeChanged) // Utilise imageSizeChanged de SGSImageItem

public:
    /**
     * @brief Constructs a QMLSGSImageItem.
     * @param parent Optional parent QQuickItem.
     */
    explicit QMLSGSImageItem(QQuickItem* parent = nullptr);

    /**
     * @brief Destroys the QMLSGSImageItem.
     */
    ~QMLSGSImageItem(); // Utilise override et = default si le destructeur de base suffit
};

} // namespace CaptureMoment::UI
