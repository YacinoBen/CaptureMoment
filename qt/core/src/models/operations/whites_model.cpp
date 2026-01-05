/**
 * @file whites_model.cpp
 * @brief Implementation of WhitesModel inheriting from BaseAdjustmentModel
 * @author CaptureMoment Team
 * @date 2025
 */

#include "models/operations/whites_model.h"
#include "controller/image_controller_base.h"
#include <spdlog/spdlog.h>

namespace CaptureMoment::UI::Models::Operations {

// Constructor: Initializes the model with a default value of 0.0.
WhitesModel::WhitesModel(QObject* parent)
    : BaseAdjustmentModel(parent)
{
    spdlog::debug("WhitesModel: Created (inherits default value from BaseAdjustmentModel)");
}

Core::Operations::OperationDescriptor WhitesModel::getDescriptor() const
{
    Core::Operations::OperationDescriptor descriptor;
    descriptor.type = Core::Operations::OperationType::Whites;
    descriptor.name = "Whites (" + std::to_string(static_cast<int>(m_params.value * 100)) + "%)";
    descriptor.enabled = true;
    descriptor.setParam<float>("value", m_params.value);
    return descriptor;
}

// Sets the ImageControllerBase reference used for applying operations.
void WhitesModel::setImageController(Controller::ImageControllerBase* controller)
{
    m_image_controller = controller;
    if (!m_image_controller)
    {
        spdlog::warn("WhitesModel: ImageControllerBase set to nullptr");
        return;
    }
    spdlog::debug("WhitesModel: ImageControllerBase set");

    // Register this model with the controller
    m_image_controller->registerModel(this);

    // Connect to controller's feedback signal
    connect(m_image_controller, &Controller::ImageControllerBase::operationCompleted,
            this, &WhitesModel::onOperationCompleted);

    connect(m_image_controller, &Controller::ImageControllerBase::operationFailed,
            this, &WhitesModel::onOperationFailed);
    spdlog::debug("WhitesModel: Connected to ImageControllerBase signals");
}

// Resets the whites value to its default (0.0).
void WhitesModel::reset()
{
    spdlog::debug("WhitesModel::reset: Resetting to default value (0.0)");
    // Appelle setValue de BaseAdjustmentModel
    setValue(0.0f);
}

// Handles successful operation completion from ImageControllerBase.
void WhitesModel::onOperationCompleted()
{
    spdlog::debug("WhitesModel: Operation Completed successfully");
    emit operationApplied();
}

// Handles operation failure from ImageControllerBase.
void WhitesModel::onOperationFailed(const QString& error)
{
    spdlog::error("WhitesModel: Operation failed - {}", error.toStdString());
    emit operationFailed(error);
}

} // namespace CaptureMoment::UI::Models::Operations
