/**
 * @file image_controller_sgs.cpp
 * @brief Implementation of ImageControllerSGS
 * @author CaptureMoment Team
 * @date 2025
 */

#include "controller/image_controller_sgs.h"
#include "rendering/sgs_image_item.h"
#include <spdlog/spdlog.h>

namespace CaptureMoment::UI::Controller {

// Constructor: Initializes the SGS image controller.
ImageControllerSGS::ImageControllerSGS(QObject* parent)
    : Controller::ImageControllerBase(parent) {
    spdlog::info("ImageControllerSGS: Initialized with PhotoEngine");
}

// Destructor: Cleans up resources and stops the worker thread.
ImageControllerSGS::~ImageControllerSGS()
{
    spdlog::debug("ImageControllerSGS: Destroyed, worker thread stopped");
}

// Sets the SGS image item to be used for rendering.
void ImageControllerSGS::setSGSImageItem(CaptureMoment::UI::Rendering::SGSImageItem* item)
{
    m_sgs_image_item = item;

    // Connect the rendering item to the display manager if available.
    if (m_display_manager) // Use DisplayManager like Painted
    {
        // The DisplayManager handles rendering, so give it the SGS item.
        m_display_manager->setRenderingItem(m_sgs_image_item); // Ensure IRenderingItemBase is compatible or SGSImageItem implements it
        emit sgsImageItemChanged();
        if (m_sgs_image_item) {
            spdlog::info("ImageControllerSGS: SGSImageItem connected");
        } else {
            spdlog::debug("ImageControllerSGS: SGSImageItem disconnected (set to nullptr)");
        }
    } else {
        spdlog::warn("ImageControllerSGS: DisplayManager not available, cannot connect SGSImageItem");
    }
}

// Sets the SGS image item from QML.
void ImageControllerSGS::setSGSImageItemFromQml(CaptureMoment::UI::Rendering::SGSImageItem* item)
{
    if (item) {
        setSGSImageItem(item);
        spdlog::info("ImageControllerSGS: SGSImageItem connected from QML");
    } else {
        spdlog::warn("ImageControllerSGS: Received nullptr SGSImageItem from QML");
    }
}

} // namespace CaptureMoment::UI::Controller

