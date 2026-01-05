/**
 * @file base_adjustment_model.h
 * @brief Generic Qt base class for single-value adjustment operations (e.g., Brightness, Contrast, Exposure).
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include "models/operations/operation_provider.h"
#include "domain/operation_parameters.h"

#include <QObject>
#include <QString>

namespace CaptureMoment::UI {

namespace Models::Operations {

/**
 * @brief Generic base class for operations with a single adjustable value (e.g., brightness, contrast).
 *
 * This class provides common Qt/QML properties and methods for operations that adjust a single parameter.
 * It uses RelativeAdjustmentParams internally to manage the value, its range, and activity state.
 * It inherits from OperationProvider to gain QObject functionality and implement IOperationModel.
 *
 * Derived classes must still implement:
 * - name() const override
 * - getType() const override
 * - getDescriptor() const override (or provide a mechanism to generate it using m_params)
 * - setImageController override
 * - reset() override
 * - onOperationCompleted/Failed overrides
 *
 * But they inherit the Q_PROPERTY definitions, value/minimum/maximum/isActive methods, and Qt infrastructure.
 */
class BaseAdjustmentModel : public OperationProvider {
    Q_OBJECT

    // Expose the 'value' property to QML, allowing two-way binding.
    // This property is backed by the 'value()' getter and 'setValue()' setter methods.
    // Changes trigger the 'valueChanged' signal.
    Q_PROPERTY(float value READ value WRITE setValue NOTIFY valueChanged)

    // Expose the constant minimum value to QML.
    // This property is backed by the 'minimum()' getter method and is marked as CONSTANT,
    // meaning its value does not change during the lifetime of the object.
    Q_PROPERTY(float minimum READ minimum CONSTANT)

    // Expose the constant maximum value to QML.
    // This property is backed by the 'maximum()' getter method and is marked as CONSTANT,
    // meaning its value does not change during the lifetime of the object.
    Q_PROPERTY(float maximum READ maximum CONSTANT)

    // Expose the operation's name to QML.
    // This property is backed by the 'name()' getter method and is marked as CONSTANT.
    // Note: Uses the pure virtual name() method from derived classes.
    Q_PROPERTY(QString name READ name CONSTANT)

    // Expose the operation's activity state to QML.
    // This property is backed by the 'isActive()' getter method and emits 'isActiveChanged' when it changes.
    Q_PROPERTY(bool active READ isActive NOTIFY isActiveChanged)

protected:
    /**
     * @brief Structure holding the specific parameter for single-value adjustments.
     * Uses RelativeAdjustmentParams which is suitable for adjustments in a range like [-1.0, 1.0].
     */
    Domain::RelativeAdjustmentParams m_params;

public:
    /**
     * @brief Constructs a BaseAdjustmentModel.
     * @param parent Optional parent QObject.
     */
    explicit BaseAdjustmentModel(QObject* parent = nullptr);

    /**
     * @brief Virtual destructor.
     */
    virtual ~BaseAdjustmentModel() = default;

    /**
     * @brief Checks if the adjustment operation is currently active.
     * An operation is considered active if its internal parameters (m_params) indicate a change
     * from the default/no-op state (e.g., value is not zero).
     * @return true if the operation is active, false otherwise.
     */
    [[nodiscard]] bool isActive() const override { return m_params.isActive(); } // Implemented here

    /**
     * @brief Gets the current adjustment value.
     * @return The current value.
     */
    [[nodiscard]] float value() const { return m_params.value; }

    /**
     * @brief Gets the minimum allowed value.
     * @return float -1.0f, as defined by RelativeAdjustmentParams::MIN_VALUE.
     */
    [[nodiscard]] constexpr float minimum() const { return Domain::RelativeAdjustmentParams::MIN_VALUE; }

    /**
     * @brief Gets the maximum allowed value.
     * @return float 1.0f, as defined by RelativeAdjustmentParams::MAX_VALUE.
     */
    [[nodiscard]] constexpr float maximum() const { return Domain::RelativeAdjustmentParams::MAX_VALUE; }

public slots:
    /**
     * @brief Sets the adjustment value and triggers an update.
     *
     * This slot is intended to be called from QML when the user adjusts the value.
     * It validates the input, updates the internal RelativeAdjustmentParams structure,
     * and signals the ImageControllerBase to apply the new value.
     *
     * @param val The new value (clamped between minimum() and maximum()).
     */
    void setValue(float val);

signals:
    /**
     * @brief Signal emitted when the adjustment value changes.
     * QML can connect to this signal to react to value updates (e.g., refresh display).
     * ImageControllerBase should connect to this signal to trigger processing.
     * @param value The new value of the parameter.
     */
    void valueChanged(float value);
};

} // namespace Models::Operations

} // namespace CaptureMoment::UI
