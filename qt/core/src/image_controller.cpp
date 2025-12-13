/**
 * @file image_controller.cpp
 * @brief Implementation of ImageController
 * @author CaptureMoment Team
 * @date 2025
 */

#include "image_controller.h"
#include <spdlog/spdlog.h>
#include <QMetaObject>
#include <Qt>

namespace CaptureMoment::UI {

ImageController::ImageController(QObject* parent)
    : QObject(parent) {
    
    // Create Core components
    auto source = std::make_shared<SourceManager>();
    auto factory = std::make_shared<OperationFactory>();
    
    // Create PhotoEngine (factory is configured elsewhere)
    m_engine = std::make_shared<PhotoEngine>(source, factory);
    
    spdlog::info("ImageController: Initialized");
}

ImageController::~ImageController() {
    m_worker_thread.quit();
    m_worker_thread.wait();
    spdlog::debug("ImageController: Destroyed");
}

void ImageController::setRHIImageItem(Rendering::RHIImageItem* item) {
    m_rhi_image_item = item;
    spdlog::debug("ImageController: RHIImageItem set");
}

void ImageController::loadImage(const QString& filePath) {
    if (filePath.isEmpty()) {
        emit imageLoadFailed("Empty file path");
        return;
    }
    
    spdlog::info("ImageController::loadImage: {}", filePath.toStdString());

    // Run on worker thread
    QMetaObject::invokeMethod(this, [this, filePath]() {
        doLoadImage(filePath);
    }, Qt::QueuedConnection);
}

void ImageController::applyOperations(const std::vector<OperationDescriptor>& operations) {
    if (!m_current_image) {
        emit operationFailed("No image loaded");
        return;
    }
    
    if (operations.empty()) {
        spdlog::warn("ImageController::applyOperations: Empty operation list");
        emit operationFailed("No operations specified");
        return;
    }
    
    spdlog::info("ImageController::applyOperations: {} operation(s)", operations.size());
    
    // Run on worker thread
    QMetaObject::invokeMethod(this, [this, operations]() {
        doApplyOperations(operations);
    }, Qt::QueuedConnection);
}

void ImageController::doLoadImage(const QString& filePath) {
    // Load image via PhotoEngine
    if (!m_engine->loadImage(filePath.toStdString())) {
        onImageLoadResult(false, "Failed to load image");
        return;
    }
    
    // Get image metadata
    m_image_width = m_engine->width();
    m_image_height = m_engine->height();
    
    // Get tile (full image for now)
    auto task = m_engine->createTask({}, 0, 0, m_image_width, m_image_height);
    if (!task) {
        onImageLoadResult(false, "Failed to create task");
        return;
    }
    
    m_engine->submit(task);
    m_current_image = task->result();
    
    // Update RHI display
    if (m_rhi_image_item && m_current_image) {
        m_rhi_image_item->setImage(m_current_image);
    }
    
    onImageLoadResult(true, "");
}

void ImageController::doApplyOperations(const std::vector<OperationDescriptor>& operations) {
    if (!m_current_image || !m_engine) {
        onOperationResult(false, "No image loaded");
        return;
    }
    
    // Create and submit task with operations
    auto task = m_engine->createTask(operations, 0, 0, m_image_width, m_image_height);
    if (!task) {
        onOperationResult(false, "Failed to create operation task");
        return;
    }
    
    if (!m_engine->submit(task)) {
        onOperationResult(false, "Failed to submit operation task");
        return;
    }
    
    // Get result
    auto result = task->result();
    if (!result) {
        onOperationResult(false, "Operation execution failed");
        return;
    }
    
    // Commit result to source and update display
    if (m_engine->commitResult(task)) {
        m_current_image = result;
        
        if (m_rhi_image_item) {
            m_rhi_image_item->updateTile(result);
        }
        
        onOperationResult(true, "");
    } else {
        onOperationResult(false, "Failed to commit operation result");
    }
}

} // namespace CaptureMoment::Qt
