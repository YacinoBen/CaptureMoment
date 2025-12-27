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

// Loads an image file using the PhotoEngine and initializes the display.
void ImageControllerSGS::doLoadImage(const QString& filePath)
{
    spdlog::info("ImageControllerSGS::doLoadImage: Starting load on worker thread");

    // Load image via PhotoEngine
    if (!m_engine->loadImage(filePath.toStdString()))
    {
        spdlog::error("ImageControllerSGS::doLoadImage: Failed to load image from PhotoEngine");
        onImageLoadResult(false, "Failed to load image");
        return; // Loading failed, exit.
    }

    // Get image metadata
    m_image_width = m_engine->width();
    m_image_height = m_engine->height();

    spdlog::info("ImageControllerSGS::doLoadImage: Image loaded {}x{}", m_image_width, m_image_height);

    // Retrieve the working image after loading
    if (!m_engine->getWorkingImage()) {
        spdlog::error("ImageControllerSGS::doLoadImage: Failed to get initial working image from PhotoEngine");
        onImageLoadResult(false, "Failed to get initial working image");
        return; // Failed to retrieve working image, exit.
    }

    // Use DisplayManager to create the display image
    if (m_display_manager)
    {
        spdlog::info("ImageControllerSGS: Creating display image via DisplayManager");
        m_display_manager->createDisplayImage(m_engine->getWorkingImage()); // Sends the full working image
        spdlog::debug("ImageControllerSGS: DisplayManager updated (auto-sent to SGSImageItem)");
    }
    else
    {
        spdlog::error("ImageControllerSGS: No DisplayManager available during load!");
        onImageLoadResult(false, "DisplayManager not available");
        return; // No DisplayManager, cannot display, exit.
    }

    onImageLoadResult(true, "");
}

void ImageControllerSGS::doApplyOperations(const std::vector<Core::Operations::OperationDescriptor>& operations)
{
    spdlog::debug("ImageControllerSGS::doApplyOperations: Starting operation processing");

    if (!m_engine)
    {
        spdlog::error("ImageControllerSGS::doApplyOperations: No PhotoEngine available!");
        onOperationResult(false, "No engine or image loaded");
        return; // No engine, exit.
    }

    // 1. Create and submit task with operations (apply to the full image)
    auto task = m_engine->createTask(operations, 0, 0, m_image_width, m_image_height);
    if (!task)
    {
        spdlog::error("ImageControllerSGS::doApplyOperations: Failed to create operation task");
        onOperationResult(false, "Failed to create operation task");
        return; // Task creation failed, exit.
    }

    spdlog::debug("ImageControllerSGS::doApplyOperations: Task created, submitting to PhotoEngine");

    if (!m_engine->submit(task))
    {
        spdlog::error("ImageControllerSGS::doApplyOperations: Failed to submit operation task");
        onOperationResult(false, "Failed to submit operation task");
        return; // Task submission failed, exit.
    }

    // 2. Get result (this is the processed tile/region - normally the full image here)
    auto result = task->result();
    if (!result)
    {
        spdlog::error("ImageControllerSGS::doApplyOperations: Operation execution failed, no result from task");
        onOperationResult(false, "Operation execution failed");
        return; // Execution failed, exit.
    }

    spdlog::info("ImageControllerSGS::doApplyOperations: Operation succeeded, committing result to working image");

    // 3. Commit result to PhotoEngine's working image (NOT to SourceManager)
    if (m_engine->commitResult(task)) // Commit the result into PhotoEngine
    {
        spdlog::info("ImageControllerSGS::doApplyOperations: Result committed to working image");
        // 4. Retrieve the updated full working image from PhotoEngine
        auto updatedWorkingImage = m_engine->getWorkingImage();

        if (!updatedWorkingImage) {
            spdlog::error("ImageControllerSGS::doApplyOperations: Failed to get updated working image from PhotoEngine after commit.");
            onOperationResult(false, "Failed to get updated image");
            return; // Failed to retrieve updated image, exit.
        }

        // 5. Update display with the updated working image (via DisplayManager)
        if(m_display_manager) {
            spdlog::debug("ImageControllerSGS::doApplyOperations: Updating DisplayManager with new working image result");
            // DisplayManager downsamples and calls updateTile on the rendering item (SGSImageItem)
            m_display_manager->updateDisplayTile(updatedWorkingImage); // Sends the updated working image
            spdlog::info("ImageControllerSGS::doApplyOperations: DisplayManager updated with new working image result");
        } else {
            spdlog::warn("ImageControllerSGS::doApplyOperations: No DisplayManager set, cannot update display.");
            // Even if display fails, the core operation is successful.
        }

        onOperationResult(true, ""); // Operation successful (or at least the core part is)
    }
    else
    {
        spdlog::error("ImageControllerSGS::doApplyOperations: Failed to commit result to working image in PhotoEngine");
        onOperationResult(false, "Failed to commit operation result to working image");
        // Commit failed, exit.
    }
}

} // namespace CaptureMoment::UI::Controller

