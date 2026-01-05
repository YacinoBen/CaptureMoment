/**
 * @file contrast_model.cpp
 * @brief Implementation of ContrastModel inheriting from BaseAdjustmentModel
 * @author CaptureMoment Team
 * @date 2025
 */

#include "models/operations/basic_adjustment_models/contrast_model.h"
#include "controller/image_controller_base.h"
#include <spdlog/spdlog.h>

namespace CaptureMoment::UI::Models::Operations {

// Constructor: Initializes the model with a default value of 0.0.
ContrastModel::ContrastModel(QObject* parent)
    : BaseAdjustmentModel(parent)
{
    spdlog::debug("ContrastModel: Created (inherits default value from BaseAdjustmentModel)");
}

Core::Operations::OperationDescriptor ContrastModel::getDescriptor() const
{
    Core::Operations::OperationDescriptor descriptor;
    descriptor.type = Core::Operations::OperationType::Contrast;
    descriptor.name = "Contrast (" + std::to_string(static_cast<int>(m_params.value * 100)) + "%)";
    descriptor.enabled = true;
    descriptor.setParam<float>("value", m_params.value);
    return descriptor;
}

// Sets the ImageControllerBase reference used for applying operations.
void ContrastModel::setImageController(Controller::ImageControllerBase* controller)
{
    m_image_controller = controller;
    if (!m_image_controller)
    {
        spdlog::warn("ContrastModel: ImageControllerBase set to nullptr");
        return;
    }
    spdlog::debug("ContrastModel: ImageControllerBase set");

    // Register this model with the controller
    m_image_controller->registerModel(this);

    // Connect to controller's feedback signal
    connect(m_image_controller, &Controller::ImageControllerBase::operationCompleted,
            this, &ContrastModel::onOperationCompleted);

    connect(m_image_controller, &Controller::ImageControllerBase::operationFailed,
            this, &ContrastModel::onOperationFailed);
    spdlog::debug("ContrastModel: Connected to ImageControllerBase signals");
}

// Resets the contrast value to its default (0.0).
void ContrastModel::reset()
{
    spdlog::debug("ContrastModel::reset: Resetting to default value (0.0)");
    // Appelle setValue de BaseAdjustmentModel
    setValue(0.0f);
}

// Handles successful operation completion from ImageControllerBase.
void ContrastModel::onOperationCompleted()
{
    spdlog::debug("ContrastModel: Operation Completed successfully");
    emit operationApplied();
}

// Handles operation failure from ImageControllerBase.
void ContrastModel::onOperationFailed(const QString& error)
{
    spdlog::error("ContrastModel: Operation failed - {}", error.toStdString());
    emit operationFailed(error);
}

} // namespace CaptureMoment::UI::Models::Operations
