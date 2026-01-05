/**
 * @file highlights_model.cpp
 * @brief Implementation of HighlightsModel inheriting from BaseAdjustmentModel
 * @author CaptureMoment Team
 * @date 2025
 */

#include "models/operations/basic_adjustment_models/highlights_model.h"
#include "controller/image_controller_base.h"
#include <spdlog/spdlog.h>

namespace CaptureMoment::UI::Models::Operations {

// Constructor: Initializes the model with a default value of 0.0.
HighlightsModel::HighlightsModel(QObject* parent)
    : BaseAdjustmentModel(parent)
{
    spdlog::debug("HighlightsModel: Created (inherits default value from BaseAdjustmentModel)");
}

Core::Operations::OperationDescriptor HighlightsModel::getDescriptor() const
{
    Core::Operations::OperationDescriptor descriptor;
    descriptor.type = Core::Operations::OperationType::Highlights;
    descriptor.name = "Highlights (" + std::to_string(static_cast<int>(m_params.value * 100)) + "%)";
    descriptor.enabled = true;
    descriptor.setParam<float>("value", m_params.value);
    return descriptor;
}

// Sets the ImageControllerBase reference used for applying operations.
void HighlightsModel::setImageController(Controller::ImageControllerBase* controller)
{
    m_image_controller = controller;
    if (!m_image_controller)
    {
        spdlog::warn("HighlightsModel: ImageControllerBase set to nullptr");
        return;
    }
    spdlog::debug("HighlightsModel: ImageControllerBase set");

    // Register this model with the controller
    m_image_controller->registerModel(this);

    // Connect to controller's feedback signal
    connect(m_image_controller, &Controller::ImageControllerBase::operationCompleted,
            this, &HighlightsModel::onOperationCompleted);

    connect(m_image_controller, &Controller::ImageControllerBase::operationFailed,
            this, &HighlightsModel::onOperationFailed);
    spdlog::debug("HighlightsModel: Connected to ImageControllerBase signals");
}

// Resets the highlights value to its default (0.0).
void HighlightsModel::reset()
{
    spdlog::debug("HighlightsModel::reset: Resetting to default value (0.0)");
    // Appelle setValue de BaseAdjustmentModel
    setValue(0.0f);
}

// Handles successful operation completion from ImageControllerBase.
void HighlightsModel::onOperationCompleted()
{
    spdlog::debug("HighlightsModel: Operation Completed successfully");
    emit operationApplied();
}

// Handles operation failure from ImageControllerBase.
void HighlightsModel::onOperationFailed(const QString& error)
{
    spdlog::error("HighlightsModel: Operation failed - {}", error.toStdString());
    emit operationFailed(error);
}

} // namespace CaptureMoment::UI::Models::Operations
