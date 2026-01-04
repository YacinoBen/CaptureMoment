/**
 * @file image_controller_painted.cpp
 * @brief Implementation of ImageControllerPainted
 * @author CaptureMoment Team
 * @date 2025
 */

#include "controller/image_controller_painted.h"
#include "rendering/painted_image_item.h"

#include <spdlog/spdlog.h>

namespace CaptureMoment::UI::Controller {

ImageControllerPainted::ImageControllerPainted(QObject* parent)
    : Controller::ImageControllerBase(parent) { 
    spdlog::info("ImageControllerPainted: Initialized with PhotoEngine");
}

ImageControllerPainted::~ImageControllerPainted()
{
    spdlog::debug("ImageControllerPainted: Destroyed, worker thread stopped");
}

void ImageControllerPainted::setPaintedImageItem(CaptureMoment::UI::Rendering::PaintedImageItem* item)
{
    m_painted_image_item = item; 

    if (m_display_manager)
    {
        m_display_manager->setRenderingItem(m_painted_image_item);
        emit paintedImageItemChanged();
        if (m_painted_image_item) {
            spdlog::info("ImageControllerPainted: PaintedImageItem connected");
        } else {
            spdlog::debug("ImageControllerPainted: PaintedImageItem disconnected (set to nullptr)");
        }
    } else {
         spdlog::warn("ImageControllerPainted: DisplayManager not available, cannot connect PaintedImageItem");
    }
}

void ImageControllerPainted::setPaintedImageItemFromQml(CaptureMoment::UI::Rendering::PaintedImageItem* item)
{
    if (item) {
        setPaintedImageItem(item); 
        spdlog::info("ImageControllerPainted: PaintedImageItem connected from QML");
    } else {
        spdlog::warn("ImageControllerPainted: Received nullptr PaintedImageItem from QML");
    }
}

} // namespace CaptureMoment::UI::Controller
