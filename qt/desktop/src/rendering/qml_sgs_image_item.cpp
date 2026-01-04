/**
 * @file qml_sgs_image_item.cpp
 * @brief Implementation of QMLSGSImageItem using modern C++20/23 features.
 * @author CaptureMoment Team
 * @date 2025
 */

#include "rendering/qml_sgs_image_item.h" // Include the corresponding header
#include <spdlog/spdlog.h>

namespace CaptureMoment::UI {

// Constructor: Initializes the item.
QMLSGSImageItem::QMLSGSImageItem(QQuickItem* parent)
    : Rendering::SGSImageItem(parent) // Call the base SGSImageItem constructor
{
    spdlog::debug("QMLSGSImageItem: Created (inherits SGSImageItem logic)");
}

// Destructor: Cleans up resources.
QMLSGSImageItem::~QMLSGSImageItem() {
    spdlog::debug("QMLSGSImageItem: Destroyed");
    // The SGSImageItem destructor is called automatically.
}

} // namespace CaptureMoment::UI
