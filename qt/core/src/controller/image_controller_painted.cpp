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
    spdlog::debug("ImageController::doLoadImage: Starting load on worker thread");
    
    // Load image via PhotoEngine
    if (!m_engine->loadImage(filePath.toStdString())) {
        onImageLoadResult(false, "Failed to load image");
        return;
    }
    
    // Get image metadata
    m_image_width = m_engine->width();
    m_image_height = m_engine->height();
    
    spdlog::info("ImageController::doLoadImage: Image loaded {}x{}", 
                  m_image_width, m_image_height);

    // Get tile (full image for now)
    auto task = m_engine->createTask({}, 0, 0, m_image_width, m_image_height);
    if (!task) {
        onImageLoadResult(false, "Failed to create task");
        return;
    }
    
    if (!m_engine->submit(task)) {
        onImageLoadResult(false, "Failed to submit task");
        return;
    }
    
    m_current_image = task->result();
    if (!m_current_image) {
        onImageLoadResult(false, "Failed to get image result");
        return;
    }
    

    if (m_display_manager) {
        spdlog::info("ImageControllerPainted: Creating display image via DisplayManager");
        m_display_manager->createDisplayImage(m_current_image);
        // DisplayManager automatically calls m_painted_image_item->setImage() with downsampled image
        spdlog::debug("ImageControllerPainted: DisplayManager updated (auto-sent to PaintedImageItem)");
    } else {
        spdlog::error("ImageControllerPainted: No DisplayManager!");
    }
    
    onImageLoadResult(true, "");
}

void ImageControllerPainted::doApplyOperations(const std::vector<OperationDescriptor>& operations)
{
    spdlog::debug("ImageController::doApplyOperations: Starting operation processing");
    
    if (!m_current_image || !m_engine) {
        onOperationResult(false, "No image loaded");
        return;
    }
    
    // Create and submit task with operations
    auto task = m_engine->createTask(operations, 0, 0, m_image_width, m_image_height);
    if (!task) {
        onOperationResult(false, "Failed to create operation task");
        spdlog::error("ImageController::doApplyOperations: Failed to create task");
        return;
    }
    
    spdlog::debug("ImageController::doApplyOperations: Task created, submitting to PhotoEngine");
    
    if (!m_engine->submit(task)) {
        onOperationResult(false, "Failed to submit operation task");
        spdlog::error("ImageController::doApplyOperations: Failed to submit task");
        return;
    }
    
    // Get result
    auto result = task->result();
    if (!result) {
        onOperationResult(false, "Operation execution failed");
        spdlog::error("ImageController::doApplyOperations: Operation failed");
        return;
    }
    
    spdlog::debug("ImageController::doApplyOperations: Operation succeeded, committing result");

    // Commit result to source and update display
    if (m_engine->commitResult(task)) {
        m_current_image = result;

        if (m_painted_image_item) {
            m_painted_image_item->updateTile(result);
            spdlog::debug("ImageControllerPainted::doApplyOperations: RHIImageItem updated with new result");
        }
        
        onOperationResult(true, "");
    } else {
        onOperationResult(false, "Failed to commit operation result");
        spdlog::error("ImageControllerPainted::doApplyOperations: Failed to commit result");
    }
}

} // namespace CaptureMoment::UI
