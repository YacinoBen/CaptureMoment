/**
 * @file highlights_model.cpp
 * @brief Implementation of HighlightsModel inheriting from BaseAdjustmentModel
 * @author CaptureMoment Team
 * @date 2025
 */

#include "models/operations/basic_adjustment_models/highlights_model.h"
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

// Resets the highlights value to its default (0.0).
void HighlightsModel::reset()
{
    spdlog::debug("HighlightsModel::reset: Resetting to default value (0.0)");
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
