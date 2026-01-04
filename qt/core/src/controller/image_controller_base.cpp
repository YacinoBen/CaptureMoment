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
    : QObject(parent)
{
    
    // Create Core components
    auto source = std::make_shared<Core::Managers::SourceManager>();
    auto factory = std::make_shared<Core::Operations::OperationFactory>();

    // Create the display manager
    m_display_manager = std::make_unique<CaptureMoment::UI::Display::DisplayManager>(this);
    
    // Register all operations (Brightness, Contrast, etc.)
    Core::Operations::OperationRegistry::registerAll(*factory);
    
    // Create PhotoEngine with registered operations
    m_engine = std::make_shared<Core::Engine::PhotoEngine>(source, factory);
    
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
    
    spdlog::info("ImageControllerBase::loadImage: calling method-thread doLoadImage() Loading {}", filePath.toStdString());

    // Run on worker thread to avoid blocking UI
    QMetaObject::invokeMethod(this, [this, filePath]() {
        doLoadImage(filePath);
    }, Qt::QueuedConnection);
}

void ImageControllerBase::applyOperations(const std::vector<Core::Operations::OperationDescriptor>& operations)
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

void ImageControllerBase::doLoadImage(const QString& file_path)
{
    spdlog::info("ImageControllerBase::doLoadImage: Starting load on worker thread");

    // Load image via PhotoEngine
    if (!m_engine->loadImage(file_path.toStdString())) {
        onImageLoadResult(false, "Failed to load image");
        return;
    }

    // Get image metadata
    m_image_width = m_engine->width();
    m_image_height = m_engine->height();

    spdlog::info("ImageControllerBase::doLoadImage: Image loaded {}x{}",
                 m_image_width, m_image_height);;

    if (!m_engine->getWorkingImage()) {
        onImageLoadResult(false, "Failed to get image result");
        return;
    }

    if (m_display_manager)
    {
        spdlog::info("ImageControllerBase::doLoadImage Creating display image via DisplayManager");
        m_display_manager->createDisplayImage(m_engine->getWorkingImage());
        spdlog::debug("ImageControllerBase::doLoadImage DisplayManager updated (auto-sent to PaintedImageItem)");
    } else {
        spdlog::error("ImageControllerBase::doLoadImage No DisplayManager!");
    }

    onImageLoadResult(true, "");
}

void ImageControllerBase::doApplyOperations(const std::vector<Core::Operations::OperationDescriptor>& operations)
{
    spdlog::debug("ImageControllerBase::doApplyOperations: Starting operation processing");

    if (!m_engine) {
        onOperationResult(false, "No engine or image loaded");
        return;
    }

    // 1. Create and submit task with operations
    auto task = m_engine->createTask(operations, 0, 0, m_image_width, m_image_height);
    if (!task) {
        onOperationResult(false, "Failed to create operation task");
        spdlog::error("ImageControllerBase::doApplyOperations: Failed to create task");
        return;
    }

    spdlog::debug("ImageControllerBase::doApplyOperations: Task created, submitting to PhotoEngine");

    if (!m_engine->submit(task)) {
        onOperationResult(false, "Failed to submit operation task");
        spdlog::error("ImageControllerBase::doApplyOperations: Failed to submit task");
        return;
    }

    // 2. Get result (this is the processed tile/region)
    auto result = task->result();
    if (!result) {
        onOperationResult(false, "Operation execution failed");
        spdlog::error("ImageControllerBase::doApplyOperations: Operation failed");
        return;
    }

    spdlog::info("ImageControllerBase::doApplyOperations: Operation succeeded, committing result to working image");

    // 3. Commit result to PhotoEngine's working image (NOT to SourceManager)
    if (m_engine->commitResult(task))
    {
        spdlog::info("ImageControllerBase:doApplyOperations: Result committed to working image");
        // 4. Retrieve the updated full working image from PhotoEngine
        auto updated_working_image = m_engine->getWorkingImage();

        if (!updated_working_image) {
            spdlog::error("ImageControllerBase::doApplyOperations: Failed to get updated working image from PhotoEngine after commit.");
            onOperationResult(false, "Failed to get updated image");
            return;
        }

        // 5. Update display with the updated working image
        if(m_display_manager) {
            m_display_manager->updateDisplayTile(updated_working_image);
            spdlog::info("ImageControllerBase::doApplyOperations: DisplayManager updated with new working image result");
        } else {
            spdlog::warn("ImageControllerBase::doApplyOperations: No DisplayManager set, cannot update display.");
        }

        onOperationResult(true, "");
    } else {
        onOperationResult(false, "Failed to commit operation result to working image");
        spdlog::error("ImageControllerBase::doApplyOperations: Failed to commit result to working image");
    }
}

} // namespace CaptureMoment::UI::Controller
