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

ImageControllerSGS::ImageControllerSGS(QObject* parent)
    : Controller::ImageControllerBase(parent) { 
    spdlog::info("ImageControllerSGS: Initialized with PhotoEngine");
}

ImageControllerSGS::~ImageControllerSGS()
{
    spdlog::debug("ImageControllerSGS: Destroyed, worker thread stopped");
}

void ImageControllerSGS::setSGSImageItem(CaptureMoment::UI::Rendering::SGSImageItem* item)
{
     m_sgs_image_item = item; 

    if (m_display_manager)
    {
        m_display_manager->setRenderingItem(m_sgs_image_item);
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

void ImageControllerSGS::setSGSImageItemFromQml(CaptureMoment::UI::Rendering::SGSImageItem* item)
{
    if (item) {
        setSGSImageItem(item); 
        spdlog::info("ImageControllerSGS: SGSImageItem connected from QML");
    } else {
        spdlog::warn("ImageControllerSGS: Received nullptr SGSImageItem from QML");
    }
}

void ImageControllerSGS::doLoadImage(const QString& filePath)
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
    
    /* m_current_image = task->result();
    if (!m_current_image) {
        onImageLoadResult(false, "Failed to get image result");
        return;
    }
    
    // Update RHI display
    if (m_sgs_image_item && m_current_image) {
        m_sgs_image_item->setImage(m_current_image);
        spdlog::debug("ImageController::doLoadImage: RHIImageItem updated");
    }*/
    
    onImageLoadResult(true, "");
}

void ImageControllerSGS::doApplyOperations(const std::vector<Core::Operations::OperationDescriptor>& operations)
{
    spdlog::debug("ImageController::doApplyOperations: Starting operation processing");
    
  /*  if (!m_current_image || !m_engine) {
        onOperationResult(false, "No image loaded");
        return;
    }*/
    
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
        //m_current_image = result;
        
        if (m_sgs_image_item) {
            m_sgs_image_item->updateTile(result);
            spdlog::debug("ImageControllerSGS::doApplyOperations: RHIImageItem updated with new result");
        }
        
        onOperationResult(true, "");
    } else {
        onOperationResult(false, "Failed to commit operation result");
        spdlog::error("ImageControllerSGS::doApplyOperations: Failed to commit result");
    }
}

} // namespace CaptureMoment::UI
