/**
 * @file qml_rhi_image_item.cpp
 * @brief Implementation of QML QMLRHIImageItem
 * @author CaptureMoment Team
 * @date 2025
 */

#include <spdlog/spdlog.h>

#include "rendering/qml_rhi_image_item.h"

namespace CaptureMoment::UI {

QMLRHIImageItem::QMLRHIImageItem(QQuickItem* parent)
    : Rendering::RHIImageItem(parent) {
    spdlog::debug("QMLRHIImageItem: Created");
}

QMLRHIImageItem::~QMLRHIImageItem() {
    spdlog::debug("QMLRHIImageItem: Destroyed");
}

} // namespace CaptureMoment::UI
