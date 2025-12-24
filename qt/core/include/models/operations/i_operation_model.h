/**
 * @file ioperation_model.h
 * @brief Pure interface for operation models
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <QString>

#include "operations/operation_descriptor.h"
#include "operations/operation_type.h"

namespace CaptureMoment::UI {

/**
 * @brief Pure interface for operation models
 *
 * This is the CONTRACT - defines what all operations must implement
 * Independent of Qt, independent of signals/slots
 */
class IOperationModel {
public:
    virtual ~IOperationModel() = default;

    /**
     * @brief Get operation name
     */
    virtual QString name() const = 0;

    /**
     * @brief Check if operation is active (non-default)
     */
    [[no_discard]] virtual bool isActive() const = 0;

    /**
     * @brief Gets the type of this operation.
     * @return OperationType::Brightness.
     */
    virtual Core::Operations::OperationType getType() const = 0;

      /**
     * @brief Creates and returns an OperationDescriptor representing the current state of this model.
     * This descriptor contains the operation type, name, parameters (like the brightness value),
     * and other metadata required by the core processing engine (PipelineEngine).
     * @return OperationDescriptor representing the current brightness state.
     */
    virtual Core::Operations::OperationDescriptor getDescriptor() const = 0;

    /**
     * @brief Reset to default state
     */
    virtual void reset() = 0;
};

} // namespace CaptureMoment::UI
