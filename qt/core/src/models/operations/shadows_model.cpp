/**
 * @file shadows_model.cpp
 * @brief Implementation of ShadowsModel inheriting from BaseAdjustmentModel
 * @author CaptureMoment Team
 * @date 2025
 */

#include "models/operations/shadows_model.h"
#include "controller/image_controller_base.h"
#include <spdlog/spdlog.h>

namespace CaptureMoment::UI::Models::Operations {

// Constructor: Initializes the model with a default value of 0.0.
ShadowsModel::ShadowsModel(QObject* parent)
    : BaseAdjustmentModel(parent)
{
    spdlog::debug("ShadowsModel: Created (inherits default value from BaseAdjustmentModel)");
}

Core::Operations::OperationDescriptor ShadowsModel::getDescriptor() const
{
    Core::Operations::OperationDescriptor descriptor;
    descriptor.type = Core::Operations::OperationType::Shadows;
    descriptor.name = "Shadows (" + std::to_string(static_cast<int>(m_params.value * 100)) + "%)";
    descriptor.enabled = true;
    descriptor.setParam<float>("value", m_params.value);
    return descriptor;
}

// Sets the ImageControllerBase reference used for applying operations.
void ShadowsModel::setImageController(Controller::ImageControllerBase* controller)
{
    m_image_controller = controller;
    if (!m_image_controller)
    {
        spdlog::warn("ShadowsModel: ImageControllerBase set to nullptr");
        return;
    }
    spdlog::debug("ShadowsModel: ImageControllerBase set");

    // Register this model with the controller
    m_image_controller->registerModel(this);

    // Connect to controller's feedback signal
    connect(m_image_controller, &Controller::ImageControllerBase::operationCompleted,
            this, &ShadowsModel::onOperationCompleted);

    connect(m_image_controller, &Controller::ImageControllerBase::operationFailed,
            this, &ShadowsModel::onOperationFailed);
    spdlog::debug("ShadowsModel: Connected to ImageControllerBase signals");
}

// Resets the shadows value to its default (0.0).
void ShadowsModel::reset()
{
    spdlog::debug("ShadowsModel::reset: Resetting to default value (0.0)");
    // Appelle setValue de BaseAdjustmentModel
    setValue(0.0f);
}

// Handles successful operation completion from ImageControllerBase.
void ShadowsModel::onOperationCompleted()
{
    spdlog::debug("ShadowsModel: Operation Completed successfully");
    emit operationApplied();
}

// Handles operation failure from ImageControllerBase.
void ShadowsModel::onOperationFailed(const QString& error)
{
    spdlog::error("ShadowsModel: Operation failed - {}", error.toStdString());
    emit operationFailed(error);
}

} // namespace CaptureMoment::UI::Models::Operations
