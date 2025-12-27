/**
 * @file qml_sgs_image_item.h
 * @brief QML object wrapper for SGSImageItem using modern C++20/23 features.
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <QQuickItem>
#include "rendering/sgs_image_item.h"

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

public:
    /**
     * @brief Constructs a QMLSGSImageItem.
     * @param parent Optional parent QQuickItem.
     */
    explicit QMLSGSImageItem(QQuickItem* parent = nullptr);

    /**
     * @brief Destroys the QMLSGSImageItem.
     */
    ~QMLSGSImageItem();
};

} // namespace CaptureMoment::UI
