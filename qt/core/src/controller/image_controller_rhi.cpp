/**
 * @file image_controller_rhi.cpp
 * @brief Implementation of ImageControllerRHI
 * @author CaptureMoment Team
 * @date 2025
 */

#include "controller/image_controller_rhi.h"
#include "rendering/rhi_image_item.h"

#include <spdlog/spdlog.h>

namespace CaptureMoment::UI::Controller {

ImageControllerRHI::ImageControllerRHI(QObject* parent)
    : Controller::ImageControllerBase(parent) {
    spdlog::info("ImageControllerRHI: Initialized with PhotoEngine");
}

ImageControllerRHI::~ImageControllerRHI()
{
    spdlog::debug("ImageControllerRHI: Destroyed, worker thread stopped");
}

void ImageControllerRHI::setRHIImageItem(CaptureMoment::UI::Rendering::RHIImageItem* item)
{
    m_rhi_image_item = item;

    if (m_display_manager)
    {
        // The DisplayManager handles rendering, so give it the RHI item.
        m_display_manager->setRenderingItem(m_rhi_image_item); // Ensure IRenderingItemBase is compatible or RHIImageItem implements it
        emit rhiImageItemChanged();
        if (m_rhi_image_item) {
            spdlog::info("ImageControllerRHI::RHIImageItem connected and passed to DisplayManager");
        } else {
            spdlog::debug("ImageControllerRHI::RHIImageItem disconnected (set to nullptr)");
        }
    } else {
         spdlog::warn("ImageControllerRHI: DisplayManager not available, cannot connect RHIImageItem");
    }
}

void ImageControllerRHI::setRHIImageItemFromQml(CaptureMoment::UI::Rendering::RHIImageItem* item)
{
    if (item) {
        spdlog::info("ImageControllerRHI::setRHIImageItemFromQml try to connect from QML");
        setRHIImageItem(item);
        spdlog::info("ImageControllerRHI::setRHIImageItemFromQml RHIImageItem connected from QML");
    } else {
        spdlog::warn("ImageControllerRHI::setRHIImageItemFromQml Received nullptr RHIImageItem from QML");
    }
}

} // namespace CaptureMoment::UI::Controller
