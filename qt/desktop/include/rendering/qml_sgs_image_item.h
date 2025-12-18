/**
 * @file qml_sgs_image_item.h
 * @brief QML object wrapper for SGSImageItem
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <QQuickItem>
#include "rendering/sgs_image_item.h" // Inclure la classe de base

namespace CaptureMoment::UI {

    namespace Rendering {
        class SGSImageItem; // Déjà inclus via sgs_image_item.h
    }

    /**
     * @brief QQuickItem for displaying images using QSGSimpleTextureNode, directly usable in QML.
     *
     * This class inherits from SGSImageItem, exposing its properties (zoom, pan, image dimensions)
     * directly as QML properties. It simplifies the QML-side usage compared to
     * having a separate controller and linking them manually.
     * This class is intended to be instantiated directly in QML.
     */
    class QMLSGSImageItem : public Rendering::SGSImageItem // Hérite de la classe d'affichage basée sur SGS
    {
        Q_OBJECT

        // Expose properties from SGSImageItem to QML
        Q_PROPERTY(float zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
        Q_PROPERTY(QPointF pan READ pan WRITE setPan NOTIFY panChanged)
        Q_PROPERTY(int imageWidth READ imageWidth NOTIFY imageSizeChanged)
        Q_PROPERTY(int imageHeight READ imageHeight NOTIFY imageSizeChanged)

    public:
        /**
         * @brief Constructs a QMLSGSImageItem.
         * @param parent Optional parent QQuickItem.
         */
        explicit QMLSGSImageItem(QQuickItem* parent = nullptr);
        
        /**
         * @brief Destroys the QMLSGSImageItem.
         */ 
        ~QMLSGSImageItem(); // Le destructeur de la base suffit
    };

} // namespace CaptureMoment::UI