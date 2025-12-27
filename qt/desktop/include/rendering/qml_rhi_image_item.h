/**
 * @file qml_rhi_image_item.h
 * @brief QML object of RHIImageItem
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <QQuickItem>
#include "rendering/rhi_image_item.h"

namespace CaptureMoment::UI {

    
namespace Rendering {
    class RHIImageItem;
}

/**
 * @brief QQuickItem for displaying images using RHI, directly usable in QML.
 *
 * This class inherits from RHIImageItem, adding a layer of QML-specific
 * interaction methods (slots) and potentially managing its own connection
 * to the ImageController. It simplifies the QML-side usage compared to
 * having a separate controller and linking them manually.
 */

class QMLRHIImageItem : public Rendering::RHIImageItem
{
Q_OBJECT

public:
    /**
     * @brief Constructs a QMLRHIImageItem.
     * @param parent Optional parent QQuickItem.
     */
    explicit QMLRHIImageItem(QQuickItem* parent = nullptr);
    
    /**
     * @brief Destroys the QMLRHIImageItem.
     */ 
    ~QMLRHIImageItem();
    };

} // namespace CaptureMoment::UI
