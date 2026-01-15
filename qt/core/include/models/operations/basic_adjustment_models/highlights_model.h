/**
 * @file highlights_model.h
 * @brief Highlights operation model inheriting from BaseAdjustmentModel.
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include "models/operations/base_adjustment_model.h"
#include "operations/operation_ranges.h"

namespace CaptureMoment::UI {

namespace Models::Operations {

/**
 * @brief Highlights operation model inheriting common properties from BaseAdjustmentModel.
 *
 * Inherits Q_PROPERTY definitions, value/isActive methods,
 * Qt infrastructure (signals/slots), and parameter handling (m_params) from BaseAdjustmentModel.
 * Implements specific logic like name, type, descriptor, and range access.
 */
class HighlightsModel : public BaseAdjustmentModel {

public:
    explicit HighlightsModel(QObject* parent = nullptr);

    // --- Implement Pure Virtual Methods from IOperationModel / BaseAdjustmentModel ---
    [[nodiscard]] QString name() const override { return "Highlights"; }
    [[nodiscard]] Core::Operations::OperationType getType() const override { return Core::Operations::OperationType::Highlights; }
    [[nodiscard]] float minimum() const override { return Core::Operations::OperationRanges::getHighlightsMinValue(); }
    [[nodiscard]] float maximum() const override { return Core::Operations::OperationRanges::getHighlightsMaxValue(); }
    [[nodiscard]] Core::Operations::OperationDescriptor getDescriptor() const override;
    void reset() override;

    // --- Implement Virtual Methods from OperationProvider ---
    void onOperationCompleted() override;
    void onOperationFailed(const QString& error) override;
};

} // namespace Models::Operations

} // namespace CaptureMoment::UI
