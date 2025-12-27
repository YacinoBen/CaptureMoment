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

void ImageControllerPainted::doLoadImage(const QString& filePath)
{
    spdlog::info("ImageControllerPainted::doLoadImage: Starting load on worker thread");
    
    // Load image via PhotoEngine
    if (!m_engine->loadImage(filePath.toStdString())) {
        onImageLoadResult(false, "Failed to load image");
        return;
    }

    // Get image metadata
    m_image_width = m_engine->width();
    m_image_height = m_engine->height();
    
    spdlog::info("ImageControllerPainted::doLoadImage: Image loaded {}x{}",
                  m_image_width, m_image_height);;

    if (!m_engine->getWorkingImage()) {
        onImageLoadResult(false, "Failed to get image result");
        return;
    }
    
    if (m_display_manager)
    {
        spdlog::info("ImageControllerPainted: Creating display image via DisplayManager");
        m_display_manager->createDisplayImage(m_engine->getWorkingImage());
        spdlog::debug("ImageControllerPainted: DisplayManager updated (auto-sent to PaintedImageItem)");
    } else {
        spdlog::error("ImageControllerPainted: No DisplayManager!");
    }
    
    onImageLoadResult(true, "");
}

void ImageControllerPainted::doApplyOperations(const std::vector<Core::Operations::OperationDescriptor>& operations)
{
    spdlog::debug("ImageControllerPainted::doApplyOperations: Starting operation processing");

    if (!m_engine) {
        onOperationResult(false, "No engine or image loaded");
        return;
    }

    // 1. Create and submit task with operations
    auto task = m_engine->createTask(operations, 0, 0, m_image_width, m_image_height);
    if (!task) {
        onOperationResult(false, "Failed to create operation task");
        spdlog::error("ImageController::doApplyOperations: Failed to create task");
        return;
    }

    spdlog::debug("ImageController::doApplyOperations: Task created, submitting to PhotoEngine");

    if (!m_engine->submit(task)) {
        onOperationResult(false, "Failed to submit operation task");
        spdlog::error("ImageControllerPainted::doApplyOperations: Failed to submit task");
        return;
    }

    // 2. Get result (this is the processed tile/region)
    auto result = task->result();
    if (!result) {
        onOperationResult(false, "Operation execution failed");
        spdlog::error("ImageControllerPainted::doApplyOperations: Operation failed");
        return;
    }

    spdlog::info("ImageControllerPainted::doApplyOperations: Operation succeeded, committing result to working image");

    // 3. Commit result to PhotoEngine's working image (NOT to SourceManager)
    if (m_engine->commitResult(task))
    {
        spdlog::info("ImageControllerPainted::doApplyOperations: Result committed to working image");
        // 4. Retrieve the updated full working image from PhotoEngine
        auto updatedWorkingImage = m_engine->getWorkingImage();

        if (!updatedWorkingImage) {
            spdlog::error("ImageControllerPainted::doApplyOperations: Failed to get updated working image from PhotoEngine after commit.");
            onOperationResult(false, "Failed to get updated image");
            return;
        }

        // 5. Update display with the updated working image
        if(m_display_manager) {
            m_display_manager->updateDisplayTile(updatedWorkingImage);
            spdlog::info("ImageControllerPainted::doApplyOperations: DisplayManager updated with new working image result");
        } else {
            spdlog::warn("ImageControllerPainted::doApplyOperations: No DisplayManager set, cannot update display.");
        }

        onOperationResult(true, "");
    } else {
        onOperationResult(false, "Failed to commit operation result to working image");
        spdlog::error("ImageControllerPainted::doApplyOperations: Failed to commit result to working image");
    }
}

} // namespace CaptureMoment::UI::Controller
