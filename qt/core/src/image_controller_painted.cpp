/**
 * @file image_controller_painted.cpp
 * @brief Implementation of ImageControllerPainted
 * @author CaptureMoment Team
 * @date 2025
 */

#include "image_controller_painted.h"
#include "rendering/painted_image_item.h"

#include <spdlog/spdlog.h>

namespace CaptureMoment::UI {

ImageControllerPainted::ImageControllerPainted(QObject* parent)
    : ImageControllerBase(parent) { 
    spdlog::info("ImageControllerPainted: Initialized with PhotoEngine");
}

ImageControllerPainted::~ImageControllerPainted()
{
    spdlog::debug("ImageControllerPainted: Destroyed, worker thread stopped");
}

void ImageControllerPainted::setPaintedImageItem(Rendering::PaintedImageItem* item)
{
    m_painted_image_item = item;
    emit paintedImageItemChanged();
    spdlog::debug("ImageControllerPainted: PaintedImageItem set");
}

void ImageControllerPainted::setPaintedImageItemFromQml(Rendering::PaintedImageItem* item)
{
    setPaintedImageItem(item);
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
    
    spdlog::debug("ImageController::doLoadImage: Image loaded {}x{}", 
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
    
    // Update RHI display
    if (m_painted_image_item && m_current_image) {
        m_painted_image_item->setImage(m_current_image);
        spdlog::debug("ImageController::doLoadImage: RHIImageItem updated");
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
            spdlog::debug("ImageController::doApplyOperations: RHIImageItem updated with new result");
        }
        
        onOperationResult(true, "");
    } else {
        onOperationResult(false, "Failed to commit operation result");
        spdlog::error("ImageController::doApplyOperations: Failed to commit result");
    }
}


} // namespace CaptureMoment::UI
