/**
 * @file qml_sgs_image_item.cpp
 * @brief Implementation of QMLSGSImageItem
 * @author CaptureMoment Team
 * @date 2025
 */

#include "rendering/qml_sgs_image_item.h" // Inclure l'en-tête correspondant
#include <spdlog/spdlog.h>

namespace CaptureMoment::UI {

    // Constructor: Initializes the item.
    QMLSGSImageItem::QMLSGSImageItem(QQuickItem* parent)
        : Rendering::SGSImageItem(parent) // Appel du constructeur de la base SGSImageItem
    {
        spdlog::debug("QMLSGSImageItem: Created (inherits SGSImageItem logic)");
    }

    // Destructor: Cleans up resources.
    QMLSGSImageItem::~QMLSGSImageItem() {
        spdlog::debug("QMLSGSImageItem: Destroyed");
        // Le destructeur de SGSImageItem est appelé automatiquement.
    }

} // namespace CaptureMoment::UI