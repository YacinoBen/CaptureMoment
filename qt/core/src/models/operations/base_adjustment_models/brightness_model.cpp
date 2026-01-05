/**
 * @file brightness_model.cpp
 * @brief Implementation of BrightnessModel inheriting from BaseAdjustmentModel
 * @author CaptureMoment Team
 * @date 2025
 */

#include "models/operations/basic_adjustment_models/brightness_model.h"
#include "controller/image_controller_base.h"
#include <spdlog/spdlog.h>

namespace CaptureMoment::UI::Models::Operations {

// Constructor: Initializes the model with a default value of 0.0.
BrightnessModel::BrightnessModel(QObject* parent)
    : BaseAdjustmentModel(parent)
{
    spdlog::debug("BrightnessModel: Created (inherits default value from BaseAdjustmentModel)");
}

Core::Operations::OperationDescriptor BrightnessModel::getDescriptor() const
{
    Core::Operations::OperationDescriptor descriptor;
    descriptor.type = Core::Operations::OperationType::Brightness;
    descriptor.name = "Brightness (" + std::to_string(static_cast<int>(m_params.value * 100)) + "%)";
    descriptor.enabled = true;
    descriptor.setParam<float>("value", m_params.value);
    return descriptor;
}

// Sets the ImageControllerBase reference used for applying operations.
void BrightnessModel::setImageController(Controller::ImageControllerBase* controller)
{
    m_image_controller = controller;
    if (!m_image_controller)
    {
        spdlog::warn("BrightnessModel: ImageControllerBase set to nullptr");
        return;
    }
    spdlog::debug("BrightnessModel: ImageControllerBase set");

    // Register this model with the controller
    m_image_controller->registerModel(this);

    // Connect to controller's feedback signal
    connect(m_image_controller, &Controller::ImageControllerBase::operationCompleted,
            this, &BrightnessModel::onOperationCompleted);

    connect(m_image_controller, &Controller::ImageControllerBase::operationFailed,
            this, &BrightnessModel::onOperationFailed);
    spdlog::debug("BrightnessModel: Connected to ImageControllerBase signals");
}

// Resets the brightness value to its default (0.0).
void BrightnessModel::reset()
{
    spdlog::debug("BrightnessModel::reset: Resetting to default value (0.0)");
    // Appelle setValue de BaseAdjustmentModel
    setValue(0.0f);
}

// Handles successful operation completion from ImageControllerBase.
void BrightnessModel::onOperationCompleted()
{
    spdlog::debug("BrightnessModel: Operation Completed successfully");
    emit operationApplied();
}

// Handles operation failure from ImageControllerBase.
void BrightnessModel::onOperationFailed(const QString& error)
{
    spdlog::error("BrightnessModel: Operation failed - {}", error.toStdString());
    emit operationFailed(error);
}

} // namespace CaptureMoment::UI::Models::Operations
