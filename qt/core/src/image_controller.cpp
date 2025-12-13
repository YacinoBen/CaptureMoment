/**
 * @file image_controller.cpp
 * @brief Implementation of ImageController
 * @author CaptureMoment Team
 * @date 2025
 */

#include "image_controller.h"
#include "models/operations/i_operation_model.h"
#include "operations/operation_registry.h"

#include <spdlog/spdlog.h>
#include <QMetaObject>
#include <algorithm>

namespace CaptureMoment::UI {

ImageController::ImageController(QObject* parent)
    : QObject(parent) {
    
    // Create Core components
    auto source = std::make_shared<SourceManager>();
    auto factory = std::make_shared<OperationFactory>();
    
    // Register all operations (Brightness, Contrast, etc.)
    OperationRegistry::registerAll(*factory);
    
    // Create PhotoEngine with registered operations
    m_engine = std::make_shared<PhotoEngine>(source, factory);
    
    spdlog::info("ImageController: Initialized with PhotoEngine");
}

ImageController::~ImageController() {
    m_worker_thread.quit();
    m_worker_thread.wait();
    spdlog::debug("ImageController: Destroyed, worker thread stopped");
}

void ImageController::setRHIImageItem(Rendering::RHIImageItem* item) {
    m_rhi_image_item = item;
    spdlog::debug("ImageController: RHIImageItem set");
}

void ImageController::registerModel(IOperationModel* model) {
    if (!model) {
        spdlog::warn("ImageController::registerModel: Attempting to register nullptr");
        return;
    }
    
    // Check if already registered
    auto it = std::find(m_registered_models.begin(), m_registered_models.end(), model);
    if (it != m_registered_models.end()) {
        spdlog::warn("ImageController::registerModel: Model already registered");
        return;
    }
    
    // Register the model
    m_registered_models.push_back(model);
    spdlog::debug("ImageController::registerModel: Model registered. Total models: {}", 
                  m_registered_models.size());
}

void ImageController::loadImage(const QString& filePath) {
    if (filePath.isEmpty()) {
        emit imageLoadFailed("Empty file path");
        spdlog::warn("ImageController::loadImage: Empty file path");
        return;
    }
    
    spdlog::info("ImageController::loadImage: Loading {}", filePath.toStdString());

    // Run on worker thread to avoid blocking UI
    QMetaObject::invokeMethod(this, [this, filePath]() {
        doLoadImage(filePath);
    }, Qt::QueuedConnection);
}

void ImageController::applyOperations(const std::vector<OperationDescriptor>& operations) {
    if (!m_current_image) {
        emit operationFailed("No image loaded");
        spdlog::warn("ImageController::applyOperations: No image loaded");
        return;
    }
    
    if (operations.empty()) {
        emit operationFailed("No operations specified");
        spdlog::warn("ImageController::applyOperations: Empty operation list");
        return;
    }
    
    spdlog::info("ImageController::applyOperations: Applying {} operation(s)", operations.size());

    // Run on worker thread to avoid blocking UI
    QMetaObject::invokeMethod(this, [this, operations]() {
        doApplyOperations(operations);
    }, Qt::QueuedConnection);
}

void ImageController::doLoadImage(const QString& filePath) {
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
    if (m_rhi_image_item && m_current_image) {
        m_rhi_image_item->setImage(m_current_image);
        spdlog::debug("ImageController::doLoadImage: RHIImageItem updated");
    }
    
    onImageLoadResult(true, "");
}

void ImageController::doApplyOperations(const std::vector<OperationDescriptor>& operations) {
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
        
        if (m_rhi_image_item) {
            m_rhi_image_item->updateTile(result);
            spdlog::debug("ImageController::doApplyOperations: RHIImageItem updated with new result");
        }
        
        onOperationResult(true, "");
    } else {
        onOperationResult(false, "Failed to commit operation result");
        spdlog::error("ImageController::doApplyOperations: Failed to commit result");
    }
}

void ImageController::onImageLoadResult(bool success, const QString& errorMsg) {
    spdlog::debug("ImageController::onImageLoadResult: success={}", success);
    
    if (success) {
        spdlog::info("ImageController: Image loaded successfully ({}x{})", 
                     m_image_width, m_image_height);
        emit imageLoaded(m_image_width, m_image_height);
    } else {
        spdlog::error("ImageController: Image load failed - {}", errorMsg.toStdString());
        emit imageLoadFailed(errorMsg);
    }
}

void ImageController::onOperationResult(bool success, const QString& errorMsg) {
    spdlog::debug("ImageController::onOperationResult: success={}", success);
    
    if (success) {
        spdlog::info("ImageController: Operation completed successfully");
        emit operationCompleted();
    } else {
        spdlog::error("ImageController: Operation failed - {}", errorMsg.toStdString());
        emit operationFailed(errorMsg);
    }
}

} // namespace CaptureMoment::UI