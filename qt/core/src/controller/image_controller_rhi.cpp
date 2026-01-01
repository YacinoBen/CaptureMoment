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

// Loads an image file using the PhotoEngine and initializes the display.
void ImageControllerRHI::doLoadImage(const QString& filePath)
{
    spdlog::info("ImageControllerRHI::doLoadImage: Starting load on worker thread");

    // Load image via PhotoEngine
    if (!m_engine->loadImage(filePath.toStdString()))
    {
        spdlog::error("ImageControllerRHI::doLoadImage: Failed to load image from PhotoEngine");
        onImageLoadResult(false, "Failed to load image");
        return; // Loading failed, exit.
    }

    // Get image metadata
    m_image_width = m_engine->width();
    m_image_height = m_engine->height();

    spdlog::info("ImageControllerRHI::doLoadImage: Image loaded {}x{}", m_image_width, m_image_height);

    // Retrieve the working image after loading
    if (!m_engine->getWorkingImage()) {
        spdlog::error("ImageControllerRHI::doLoadImage: Failed to get initial working image from PhotoEngine");
        onImageLoadResult(false, "Failed to get initial working image");
        return; // Failed to retrieve working image, exit.
    }

    // Use DisplayManager to create the display image
    if (m_display_manager)
    {
        spdlog::info("ImageControllerRHI: Creating display image via DisplayManager");
        m_display_manager->createDisplayImage(m_engine->getWorkingImage()); // Sends the full working image
        spdlog::debug("ImageControllerRHI: DisplayManager updated (auto-sent to RHIImageItem)");
    }
    else
    {
        spdlog::error("ImageControllerRHI: No DisplayManager available during load!");
        onImageLoadResult(false, "DisplayManager not available");
        return; // No DisplayManager, cannot display, exit.
    }

    onImageLoadResult(true, "");
}

// Applies a sequence of operations to the image and updates the display.
void ImageControllerRHI::doApplyOperations(const std::vector<Core::Operations::OperationDescriptor>& operations)
{
    spdlog::debug("ImageControllerRHI::doApplyOperations: Starting operation processing");

    if (!m_engine)
    {
        spdlog::error("ImageControllerRHI::doApplyOperations: No PhotoEngine available!");
        onOperationResult(false, "No engine or image loaded");
        return; // No engine, exit.
    }

    // 1. Create and submit task with operations (apply to the full image)
    auto task = m_engine->createTask(operations, 0, 0, m_image_width, m_image_height);
    if (!task)
    {
        spdlog::error("ImageControllerRHI::doApplyOperations: Failed to create operation task");
        onOperationResult(false, "Failed to create operation task");
        return; // Task creation failed, exit.
    }

    spdlog::debug("ImageControllerRHI::doApplyOperations: Task created, submitting to PhotoEngine");

    if (!m_engine->submit(task))
    {
        spdlog::error("ImageControllerRHI::doApplyOperations: Failed to submit operation task");
        onOperationResult(false, "Failed to submit operation task");
        return; // Task submission failed, exit.
    }

    // 2. Get result (this is the processed tile/region - normally the full image here)
    auto result = task->result();
    if (!result)
    {
        spdlog::error("ImageControllerRHI::doApplyOperations: Operation execution failed, no result from task");
        onOperationResult(false, "Operation execution failed");
        return; // Execution failed, exit.
    }

    spdlog::info("ImageControllerRHI::doApplyOperations: Operation succeeded, committing result to working image");

    // 3. Commit result to PhotoEngine's working image (NOT to SourceManager)
    if (m_engine->commitResult(task)) // Commit the result into PhotoEngine
    {
        spdlog::info("ImageControllerRHI::doApplyOperations: Result committed to working image");
        // 4. Retrieve the updated full working image from PhotoEngine
        auto updatedWorkingImage = m_engine->getWorkingImage();

        if (!updatedWorkingImage) {
            spdlog::error("ImageControllerRHI::doApplyOperations: Failed to get updated working image from PhotoEngine after commit.");
            onOperationResult(false, "Failed to get updated image");
            return; // Failed to retrieve updated image, exit.
        }

        // 5. Update display with the updated working image (via DisplayManager)
        if(m_display_manager) {
            spdlog::debug("ImageControllerRHI::doApplyOperations: Updating DisplayManager with new working image result");
            // DisplayManager downsamples and calls updateTile on the rendering item (RHIImageItem)
            m_display_manager->updateDisplayTile(updatedWorkingImage); // Sends the updated working image
            spdlog::info("ImageControllerRHI::doApplyOperations: DisplayManager updated with new working image result");
        } else {
            spdlog::warn("ImageControllerRHI::doApplyOperations: No DisplayManager set, cannot update display.");
            // Even if display fails, the core operation is successful.
        }

        onOperationResult(true, ""); // Operation successful (or at least the core part is)
    }
    else
    {
        spdlog::error("ImageControllerRHI::doApplyOperations: Failed to commit result to working image in PhotoEngine");
        onOperationResult(false, "Failed to commit operation result to working image");
        // Commit failed, exit.
    }
}

} // namespace CaptureMoment::UI::Controller
