/**
 * @file image_controller_base.cpp
 * @brief Implementation of ImageControllerBase
 * @date 2025
 */

#include "controller/image_controller_base.h"

#include "models/operations/i_operation_model.h"
#include "display/display_manager.h"
#include "managers/operation_state_manager.h"
#include "models/operations/base_adjustment_model.h"
#include "common/error_handling/core_error.h"
#include <QMetaObject>
#include <algorithm>

#include <spdlog/spdlog.h>

namespace CaptureMoment::UI::Controller {

ImageControllerBase::ImageControllerBase(QObject* parent)
    : QObject(parent)
{
    spdlog::debug("ImageControllerBase: Constructing ImageControllerBase");

    m_operation_state_manager = std::make_unique<Managers::OperationStateManager>();
    spdlog::info("ImageControllerBase: Initialized OperationStateManager");

    m_operation_model_manager = std::make_unique<Models::Manager::OperationModelManager>();
    spdlog::info("ImageControllerBase: Initialized OperationModelManager");

    if (!m_operation_model_manager->createBasicAdjustmentModels())
    {
        spdlog::critical("ImageControllerBase: Failed to create basic adjustment models.");
        throw std::runtime_error("ImageControllerBase: Critical failure during model creation.");
    }

    m_display_manager = std::make_unique<Display::DisplayManager>(this);
    spdlog::info("ImageControllerBase: Initialized DisplayManager");

    m_engine = std::make_shared<Core::Engine::PhotoEngine>();
    spdlog::info("ImageControllerBase: Initialized PhotoEngine");

    connectModelsToStateManager();
    spdlog::debug("ImageControllerBase: Completed construction");
}

ImageControllerBase::~ImageControllerBase()
{
    spdlog::debug("ImageControllerBase: Destroying ImageControllerBase");
    m_worker_thread.quit();
    m_worker_thread.wait();
    spdlog::debug("ImageControllerBase: Worker thread stopped and destroyed");
}

void ImageControllerBase::registerModel(IOperationModel* model)
{
    if (!model) {
        spdlog::warn("ImageControllerBase::registerModel: Attempting to register nullptr");
        return;
    }

    if (std::find(m_registered_models.begin(), m_registered_models.end(), model) != m_registered_models.end()) {
        spdlog::debug("ImageControllerBase::registerModel: Model already registered");
        return;
    }

    m_registered_models.push_back(model);
    spdlog::debug("ImageControllerBase::registerModel: Model registered. Total models: {}", m_registered_models.size());
}

void ImageControllerBase::loadImage(const QString& file_path)
{
    if (file_path.isEmpty()) {
        spdlog::warn("ImageControllerBase::loadImage: Empty file path provided");
        emit imageLoadFailed("Empty file path");
        return;
    }

    spdlog::info("ImageControllerBase::loadImage: Calling method-thread doLoadImage() Loading {}", file_path.toStdString());

    // Run on worker thread to avoid blocking UI
    QMetaObject::invokeMethod(this, [this, file_path]() {
        doLoadImage(file_path);
    }, Qt::QueuedConnection);
}

void ImageControllerBase::loadImageFromUrl(const QUrl& file_url)
{
    if (file_url.isEmpty()) {
        spdlog::warn("ImageControllerBase::loadImageFromUrl: Empty file URL received");
        emit imageLoadFailed("Empty file URL");
        return;
    }

    QString native_path = file_url.toLocalFile();

    if (native_path.isEmpty()) {
        spdlog::warn("ImageControllerBase::loadImageFromUrl: Failed to convert URL to local file path: {}", file_url.toString().toStdString());

        if (!file_url.isLocalFile()) {
            spdlog::warn("ImageControllerBase::loadImageFromUrl: Selected URL is not a local file: {}", file_url.toString().toStdString());
            emit imageLoadFailed("Selected URL is not a local file");
            return;
        } else {
            spdlog::warn("ImageControllerBase::loadImageFromUrl: Failed to convert URL to local file path: {}", file_url.toString().toStdString());
            emit imageLoadFailed("Failed to convert URL to local file path");
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
        spdlog::warn("ImageControllerBase::applyOperations: Engine not available");
        emit operationFailed("No image loaded");
        return;
    }

    if (operations.empty())
    {
        spdlog::warn("ImageController::applyOperations: Empty operation list provided");
        emit operationFailed("No operations specified");
        return;
    }

    spdlog::info("ImageControllerBase::applyOperations: Applying {} operation(s)", operations.size());

    if (m_operation_state_manager)
    {
        auto active_ops = m_operation_state_manager->getActiveOperations();
        spdlog::debug("ImageControllerBase::applyOperations: Retrieved {} active operations", active_ops.size());

        // Run on worker thread to avoid blocking UI
        QMetaObject::invokeMethod(this, [this, active_ops]() { // Capture by value to ensure validity in worker thread
            doApplyOperations(active_ops);
        }, Qt::QueuedConnection);
    } else {
        spdlog::error("ImageControllerBase::applyOperations: OperationStateManager is null during legacy applyOperations call!");
        emit operationFailed("Internal error: OperationStateManager not initialized");
    }
}

void ImageControllerBase::doLoadImage(const QString& file_path)
{
    spdlog::info("ImageControllerBase::doLoadImage: Starting load on worker thread");

    // 1. Call Core Engine (loadFile is synchronous inside Core, but runs here in thread)
    auto load_result = m_engine->loadImage(file_path.toStdString());

    if (!load_result)
    {
        spdlog::error("ImageControllerBase::doLoadImage: Load failed for {}", file_path.toStdString());
        // Convert CoreError to QString
        auto err = load_result.error();
        QString error_msg = QString::fromStdString(
            std::format("CoreError [{}]: {}", static_cast<int>(err), Core::ErrorHandling::to_string(err))
        );
        onImageLoadResult(false, error_msg);
        return;
    }

    // 2. Get Metadata
    m_image_width = m_engine->width();
    m_image_height = m_engine->height();

    spdlog::info("ImageControllerBase::doLoadImage: Image loaded {}x{}", m_image_width, m_image_height);

    // 3. Get Working Image (UniquePtr from Core)
    // Note: PhotoEngine retourne std::expected<std::unique_ptr<...>>
    auto unique_img_region_result = m_engine->getWorkingImageAsRegion();

    if (!unique_img_region_result)
    {
        spdlog::error("ImageControllerBase::doLoadImage: Failed to get working image from engine");
        auto err = unique_img_region_result.error();
        onImageLoadResult(false, QString::fromStdString(
            std::format("Failed to get image from Engine: {}", Core::ErrorHandling::to_string(err))
        ));
        return;
    }

    spdlog::debug("ImageControllerBase::doLoadImage: Successfully retrieved working image from engine");

    // 4. UniquePtr -> SharedPtr
    std::shared_ptr<Core::Common::ImageRegion> shared_display_image = std::move(unique_img_region_result.value());

    // 5. Update DisplayManager
    if (m_display_manager)
    {
        spdlog::info("ImageControllerBase::doLoadImage: Sending image to DisplayManager");
        m_display_manager->createDisplayImage(shared_display_image);
        spdlog::debug("ImageControllerBase::doLoadImage: DisplayManager updated (auto-sent to PaintedImageItem)");
    } else {
        spdlog::error("ImageControllerBase::doLoadImage: No DisplayManager available!");
    }

    onImageLoadResult(true, "");
}

void ImageControllerBase::doApplyOperations(const std::vector<Core::Operations::OperationDescriptor>& operations)
{
    spdlog::debug("ImageControllerBase::doApplyOperations: Starting operation processing with {} operations", operations.size());

    if (!m_engine) {
        spdlog::error("ImageControllerBase::doApplyOperations: No engine available");
        onOperationResult(false, "No engine available");
        return;
    }

    // 1. Trigger Core Processing (Async inside engine)
    spdlog::debug("ImageControllerBase::doApplyOperations: Applying operations via PhotoEngine");
    m_engine->applyOperations(operations);

    // 2. Retrieve Updated Image
    // Note: PhotoEngine return std::expected<std::unique_ptr<...>>
    auto unique_img_region_result = m_engine->getWorkingImageAsRegion();

    if (!unique_img_region_result)
    {
        spdlog::error("ImageControllerBase::doApplyOperations: Failed to get updated working image from engine");
        auto err = unique_img_region_result.error();
        onOperationResult(false, QString::fromStdString(
            std::format("Failed to get updated image: {}", Core::ErrorHandling::to_string(err))
        ));
        return;
    }

    spdlog::debug("ImageControllerBase::doApplyOperations: Successfully retrieved updated working image from engine");

    // 3. UniquePtr -> SharedPtr
    std::shared_ptr<Core::Common::ImageRegion> shared_updated_image = std::move(unique_img_region_result.value());

    // 4. Update DisplayManager
    if (m_display_manager)
    {
        spdlog::debug("ImageControllerBase::doApplyOperations: Updating DisplayManager with new working image result");
        m_display_manager->updateDisplayTile(shared_updated_image);
        spdlog::info("ImageControllerBase::doApplyOperations: DisplayManager updated with new working image result");
    } else {
        spdlog::warn("ImageControllerBase::doApplyOperations: No DisplayManager set, cannot update display.");
    }

    onOperationResult(true, "");
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

void ImageControllerBase::connectModelsToStateManager()
{
    spdlog::debug("ImageControllerBase::connectModelsToStateManager: Starting model connections");

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
                                 spdlog::debug("ImageControllerBase: Value changed signal received for model {}", model->name().toStdString());

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
