/**
 * @file image_controller_base.cpp
 * @brief Implementation of ImageControllerBase
 * @author CaptureMoment Team
 * @date 2025
 */

#include "controller/image_controller_base.h"
#include "models/operations/i_operation_model.h"
#include "operations/operation_registry.h"
#include "display/display_manager.h"

#include <spdlog/spdlog.h>
#include <QMetaObject>
#include <algorithm>

namespace CaptureMoment::UI::Controller {

ImageControllerBase::ImageControllerBase(QObject* parent)
    : QObject(parent) {
    
    // Create Core components
    auto source = std::make_shared<SourceManager>();
    auto factory = std::make_shared<OperationFactory>();

    // Create the display manager
    m_display_manager = std::make_unique<CaptureMoment::UI::Display::DisplayManager>(this);
    
    // Register all operations (Brightness, Contrast, etc.)
    OperationRegistry::registerAll(*factory);
    
    // Create PhotoEngine with registered operations
    m_engine = std::make_shared<PhotoEngine>(source, factory);
    
    spdlog::info("ImageControllerBase: Initialized with PhotoEngine");
}

ImageControllerBase::~ImageControllerBase()
{
    m_worker_thread.quit();
    m_worker_thread.wait();
    spdlog::debug("ImageControllerBase: Destroyed, worker thread stopped");
}

void ImageControllerBase::registerModel(IOperationModel* model)
{
    if (!model) {
        spdlog::warn("ImageControllerBase::registerModel: Attempting to register nullptr");
        return;
    }
    
    // Check if already registered
    auto it = std::find(m_registered_models.begin(), m_registered_models.end(), model);
    if (it != m_registered_models.end()) {
        spdlog::warn("ImageControllerBase::registerModel: Model already registered");
        return;
    }
    
    // Register the model
    m_registered_models.push_back(model);
    spdlog::debug("ImageControllerBase::registerModel: Model registered. Total models: {}", 
                  m_registered_models.size());
}

void ImageControllerBase::loadImage(const QString& filePath)
{
    if (filePath.isEmpty()) {
        emit imageLoadFailed("Empty file path");
        spdlog::warn("ImageControllerBase::loadImage: Empty file path");
        return;
    }
    
    spdlog::info("ImageControllerBase::loadImage: Loading {}", filePath.toStdString());

    // Run on worker thread to avoid blocking UI
    QMetaObject::invokeMethod(this, [this, filePath]() {
        doLoadImage(filePath);
    }, Qt::QueuedConnection);
}

void ImageControllerBase::applyOperations(const std::vector<OperationDescriptor>& operations)
{
    if (!m_engine) {
        emit operationFailed("No image loaded");
        spdlog::warn("ImageControllerBase::applyOperations: Engine Error load");
        return;
    }
    
    if (operations.empty()) {
        emit operationFailed("No operations specified");
        spdlog::warn("ImageController::applyOperations: Empty operation list");
        return;
    }
    
    spdlog::info("ImageControllerBase::applyOperations: Applying {} operation(s)", operations.size());

    // Run on worker thread to avoid blocking UI
    QMetaObject::invokeMethod(this, [this, operations]() {
        doApplyOperations(operations);
    }, Qt::QueuedConnection);
}

void ImageControllerBase::onImageLoadResult(bool success, const QString& errorMsg)
{
    spdlog::debug("ImageControllerBase::onImageLoadResult: success={}", success);
    
    if (success) {
        spdlog::info("ImageControllerBase: Image loaded successfully ({}x{})", 
                     m_image_width, m_image_height);
        emit imageSizeChanged();
        emit imageLoaded(m_image_width, m_image_height);
    } else {
        spdlog::error("ImageControllerBase: Image load failed - {}", errorMsg.toStdString());
        emit imageLoadFailed(errorMsg);
    }
}

void ImageControllerBase::onOperationResult(bool success, const QString& errorMsg)
{
    spdlog::debug("ImageControllerBase::onOperationResult: success={}", success);
    
    if (success) {
        spdlog::info("ImageControllerBase: Operation completed successfully");
        emit operationCompleted();
    } else {
        spdlog::error("ImageControllerBase: Operation failed - {}", errorMsg.toStdString());
        emit operationFailed(errorMsg);
    }
}

} // namespace CaptureMoment::UI
