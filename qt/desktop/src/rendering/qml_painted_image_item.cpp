/**
 * @file qml_painted_image_item.cpp
 * @brief Implementation of QML QMLPaintedImageItem
 * @author CaptureMoment Team
 * @date 2025
 */

#include <spdlog/spdlog.h>

#include "rendering/qml_painted_image_item.h"

namespace CaptureMoment::UI {

QMLPaintedImageItem::QMLPaintedImageItem(QQuickItem* parent)
    : Rendering::PaintedImageItem(parent) {
    spdlog::debug("QMLPaintedImageItem: Created");
}

QMLPaintedImageItem::~QMLPaintedImageItem() {
    spdlog::debug("QMLPaintedImageItem: Destroyed");
}

} // namespace CaptureMoment::UI
