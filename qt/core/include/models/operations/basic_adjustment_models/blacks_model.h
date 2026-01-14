/**
 * @file blacks_model.h
 * @brief Blacks operation model inheriting from BaseAdjustmentModel.
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include "models/operations/base_adjustment_model.h"
#include "operations/operation_ranges.h"

namespace CaptureMoment::UI {

namespace Models::Operations {

/**
 * @brief Blacks operation model inheriting common properties from BaseAdjustmentModel.
 *
 * Inherits Q_PROPERTY definitions, value/isActive methods,
 * Qt infrastructure (signals/slots), and parameter handling (m_params) from BaseAdjustmentModel.
 * Implements specific logic like name, type, descriptor, and range access.
 */
class BlacksModel : public BaseAdjustmentModel {

public:
    explicit BlacksModel(QObject* parent = nullptr);

    // --- Implement Pure Virtual Methods from IOperationModel / BaseAdjustmentModel ---
    [[nodiscard]] QString name() const override { return "Blacks"; }
    [[nodiscard]] Core::Operations::OperationType getType() const override { return Core::Operations::OperationType::Blacks; }
    [[nodiscard]] float minimum() const override { return Core::Operations::OperationRanges::getBlacksMinValue(); }
    [[nodiscard]] float maximum() const override { return Core::Operations::OperationRanges::getBlacksMaxValue(); }
    [[nodiscard]] Core::Operations::OperationDescriptor getDescriptor() const override;
    void reset() override;

    // --- Implement Virtual Methods from OperationProvider ---
    void onOperationCompleted() override;
    void onOperationFailed(const QString& error) override;
};

} // namespace Models::Operations

} // namespace CaptureMoment::UI
