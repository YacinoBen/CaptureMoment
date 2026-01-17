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
#include "managers/operation_state_manager.h"

#include "models/operations/base_adjustment_model.h"
#include <QMetaObject>

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
    auto pipeline = std::make_shared<Core::Operations::OperationPipeline>();

    // Create the operation state manager
    m_operation_state_manager = std::make_unique<CaptureMoment::UI::Managers::OperationStateManager>();
    spdlog::info("ImageControllerBase: Initialized OperationStateManager");

    // Create the operation mmodel manager
    m_operation_model_manager = std::make_unique<CaptureMoment::UI::Models::Manager::OperationModelManager>();
    spdlog::info("ImageControllerBase: Initialized OperationModelManager");

    if (!m_operation_model_manager->createBasicAdjustmentModels())
    {
        spdlog::critical("ImageControllerBase: Failed to create basic adjustment models.");
        throw std::runtime_error("ImageControllerBase: Critical failure during model creation.");
    }

    // Create the display manager
    m_display_manager = std::make_unique<CaptureMoment::UI::Display::DisplayManager>(this);
    spdlog::info("ImageControllerBase: Initialized DisplayManager");

    // Register all operations (Brightness, Contrast, etc.)
    Core::Operations::OperationRegistry::registerAll(*factory);

    // Create PhotoEngine with registered operations and new dependencies
    m_engine = std::make_shared<Core::Engine::PhotoEngine>(source, factory, pipeline);
    spdlog::info("ImageControllerBase: Initialized with PhotoEngine");

    connectModelsToStateManager();
    spdlog::info("ImageControllerBase: All models connected to OperationStateManager via valueChanged signal.");
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

void ImageControllerBase::loadImage(const QString& file_path)
{
    if (file_path.isEmpty()) {
        emit imageLoadFailed("Empty file path");
        spdlog::warn("ImageControllerBase::loadImage: Empty file path");
        return;
    }

    spdlog::info("ImageControllerBase::loadImage: calling method-thread doLoadImage() Loading {}", file_path.toStdString());

    // Run on worker thread to avoid blocking UI
    QMetaObject::invokeMethod(this, [this, file_path]() {
        doLoadImage(file_path);
    }, Qt::QueuedConnection);
}


void ImageControllerBase::loadImageFromUrl(const QUrl& file_url)
{
    if (file_url.isEmpty()) {
        emit imageLoadFailed("Empty file URL");
        spdlog::warn("ImageControllerBase::loadImageFromUrl: Empty file URL received");
        return;
    }

    QString native_path = file_url.toLocalFile();

    if (native_path.isEmpty()) {

        if (!file_url.isLocalFile()) {
            emit imageLoadFailed("Selected URL is not a local file");
            spdlog::warn("ImageControllerBase::loadImageFromUrl: Selected URL is not a local file: {}", file_url.toString().toStdString());
            return;
        } else {
            emit imageLoadFailed("Failed to convert URL to local file path");
            spdlog::warn("ImageControllerBase::loadImageFromUrl: Failed to convert URL to local file path: {}", file_url.toString().toStdString());
            return;
        }
    }

    spdlog::info("ImageControllerBase::loadImageFromUrl: Converted URL to native path: {}", native_path.toStdString());

    loadImage(native_path);
}

void ImageControllerBase::applyOperations(const std::vector<Core::Operations::OperationDescriptor>& operations)
{
    if (!m_engine)
    {
        emit operationFailed("No image loaded");
        spdlog::warn("ImageControllerBase::applyOperations: Engine Error load");
        return;
    }

    if (operations.empty())
    {
        emit operationFailed("No operations specified");
        spdlog::warn("ImageController::applyOperations: Empty operation list");
        return;
    }

    spdlog::info("ImageControllerBase::applyOperations: Applying {} operation(s)", operations.size());

    if (m_operation_state_manager)
    {
        auto active_ops = m_operation_state_manager->getActiveOperations();
        // Run on worker thread to avoid blocking UI
        QMetaObject::invokeMethod(this, [this, active_ops]() { // Capture by value to ensure validity in worker thread
            doApplyOperations(active_ops);
        }, Qt::QueuedConnection);
    } else {
        spdlog::error("ImageControllerBase::applyOperations: OperationStateManager is null during legacy applyOperations call!");
        emit operationFailed("Internal error: OperationStateManager not initialized");
    }
}

void ImageControllerBase::onImageLoadResult(bool success, const QString& error_msg)
{
    spdlog::debug("ImageControllerBase::onImageLoadResult: success={}", success);

    if (success) {
        spdlog::info("ImageControllerBase: Image loaded successfully ({}x{})",
                     m_image_width, m_image_height);
        emit imageSizeChanged();
        emit imageLoaded(m_image_width, m_image_height);
    } else {
        spdlog::error("ImageControllerBase: Image load failed - {}", error_msg.toStdString());
        emit imageLoadFailed(error_msg);
    }
}

void ImageControllerBase::onOperationResult(bool success, const QString& error_msg)
{
    spdlog::debug("ImageControllerBase::onOperationResult: success={}", success);

    if (success) {
        spdlog::info("ImageControllerBase: Operation completed successfully");
        emit operationCompleted();
    } else {
        spdlog::error("ImageControllerBase: Operation failed - {}", error_msg.toStdString());
        emit operationFailed(error_msg);
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
        m_display_manager->createDisplayImage(m_engine->getWorkingImageAsRegion());
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

    // 1. Apply operations via PhotoEngine (delegated to StateImageManager)
    m_engine->applyOperations(operations);

    // 2. Retrieve the updated full working image from PhotoEngine
    auto updated_working_image = m_engine->getWorkingImageAsRegion();

    if (!updated_working_image)
    {
        spdlog::error("ImageControllerBase::doApplyOperations: Failed to get updated working image from PhotoEngine after applyOperations.");
        onOperationResult(false, "Failed to get updated image");
        return;
    }

    // 3. Update display with the updated working image
    if(m_display_manager)
    {
        m_display_manager->updateDisplayTile(updated_working_image);
        spdlog::info("ImageControllerBase::doApplyOperations: DisplayManager updated with new working image result");
    } else {
        spdlog::warn("ImageControllerBase::doApplyOperations: No DisplayManager set, cannot update display.");
    }

    onOperationResult(true, "");
}


void ImageControllerBase::connectModelsToStateManager()
{
    if (!m_operation_model_manager || !m_operation_state_manager) {
        spdlog::critical("ImageControllerBase::connectModelsToStateManager: ModelManager or StateManager is null. Cannot proceed.");
        throw std::runtime_error("ImageControllerBase: Critical failure during model-to-state-manager connection setup.");
    }

    // Use the specific list of BaseAdjustmentModel (SAFE, NO CAST NEEDED)
    const auto& models = m_operation_model_manager->getBaseAdjustmentModels();
    for (const auto& model : models) { // model is std::shared_ptr<UI::Models::Operations::BaseAdjustmentModel>
        if (model) {
            // Connect the EXISTING valueChanged signal from BaseAdjustmentModel
            // The type is known at compile time, no cast needed here.
            QObject::connect(model.get(), &UI::Models::Operations::BaseAdjustmentModel::valueChanged,
                             [this, model /* Capture the shared_ptr to the specific model */ ](float new_value) {
                                 // This lambda is called when the specific BaseAdjustmentModel's value changes.

                                 // 1. Update the OperationStateManager with the CURRENT state of this specific model
                                 if (m_operation_state_manager) {
                                     auto descriptor = model->getDescriptor(); // Get the descriptor for the specific model that changed
                                     m_operation_state_manager->addOrUpdateOperation(descriptor);
                                     spdlog::debug("ImageControllerBase: Operation '{}' updated in StateManager via valueChanged signal.", descriptor.name);
                                 } else {
                                     spdlog::warn("ImageControllerBase: Received valueChanged signal, but StateManager is null.");
                                     return; // Exit if StateManager is not available
                                 }

                                 // 2. Retrieve the full list of active operations from OperationStateManager
                                 auto active_ops = m_operation_state_manager->getActiveOperations();

                                 // 3. Schedule the update to PhotoEngine on the worker thread
                                 // Run on worker thread to avoid blocking UI
                                 QMetaObject::invokeMethod(this, [this, active_ops]() { // Capture active_ops by value
                                     spdlog::debug("ImageControllerBase: Applying {} operations to PhotoEngine from valueChanged signal.", active_ops.size());
                                     doApplyOperations(active_ops);
                                 }, Qt::QueuedConnection);

                             });
            spdlog::debug("ImageControllerBase::connectModelsToStateManager: Connected model {}", model->name().toStdString());
        }
    }
    spdlog::info("ImageControllerBase::connectModelsToStateManager: All {} BaseAdjustment models connected to StateManager via valueChanged signal.", models.size());
}

} // namespace CaptureMoment::UI::Controller
