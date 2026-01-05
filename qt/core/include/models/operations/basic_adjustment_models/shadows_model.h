/**
 * @file shadows_model.h
 * @brief Shadows operation model inheriting from BaseAdjustmentModel.
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include "models/operations/base_adjustment_model.h"

namespace CaptureMoment::UI {

namespace Controller {
class ImageControllerBase;
}

namespace Models::Operations {

/**
 * @brief Shadows operation model inheriting common properties from BaseAdjustmentModel.
 *
 * Inherits Q_PROPERTY definitions, value/minimum/maximum/isActive methods,
 * Qt infrastructure (signals/slots), and parameter handling (m_params) from BaseAdjustmentModel.
 * Only needs to implement specific logic like name, type, descriptor, etc.
 */
class ShadowsModel : public BaseAdjustmentModel {

public:
    explicit ShadowsModel(QObject* parent = nullptr);

    // --- Implement Pure Virtual Methods from IOperationModel / BaseAdjustmentModel ---
    [[nodiscard]] QString name() const override { return "Shadows"; }
    [[nodiscard]] Core::Operations::OperationType getType() const override { return Core::Operations::OperationType::Shadows; }
    [[nodiscard]] Core::Operations::OperationDescriptor getDescriptor() const override;
    void reset() override;

    // --- Implement Virtual Methods from OperationProvider ---
    void setImageController(Controller::ImageControllerBase* controller) override;
    void onOperationCompleted() override;
    void onOperationFailed(const QString& error) override;
};

} // namespace Models::Operations

} // namespace CaptureMoment::UI
