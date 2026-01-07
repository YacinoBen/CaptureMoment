/**
 * @file whites_model.cpp
 * @brief Implementation of WhitesModel inheriting from BaseAdjustmentModel
 * @author CaptureMoment Team
 * @date 2025
 */

#include "models/operations/basic_adjustment_models/whites_model.h"
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

// Resets the whites value to its default (0.0).
void WhitesModel::reset()
{
    spdlog::debug("WhitesModel::reset: Resetting to default value (0.0)");
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
