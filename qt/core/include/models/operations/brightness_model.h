/**
 * @file brightness_model.h
 * @brief Brightness operation model for QML integration, using RelativeAdjustmentParams.
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include "models/operations/operation_provider.h"
#include "domain/operation_parameters.h"

namespace CaptureMoment::UI {

/**
 * @brief Brightness operation model
 * 
 * DOUBLE INHERITANCE:
 * - IOperationModel : Pure interface contract (non-QObject)
 * - OperationProvider : Qt infrastructure (QObject with signals/slots)
 * 
 * KEY POINTS:
 * - ONLY OperationProvider is a QObject
 * - IOperationModel is pure interface (no Q_OBJECT needed)
 * - BrightnessModel ONLY has Q_OBJECT from OperationProvider
 * - BrightnessModel defines its OWN setValue() and valueChanged()
 */

class BrightnessModel :  public OperationProvider{
    Q_OBJECT
    
    // Expose the 'value' property to QML, allowing two-way binding.
    // This property is backed by the 'value()' getter and 'setValue()' setter methods.
    // Changes trigger the 'valueChanged' signal.
    Q_PROPERTY(float value READ value WRITE setValue NOTIFY valueChanged)

    // Expose the constant minimum value to QML.
    // This property is backed by the 'minimum()' getter method and is marked as CONSTANT,
    // meaning its value does not change during the lifetime of the object..
    Q_PROPERTY(float minimum READ minimum CONSTANT)

    // Expose the constant maximum value to QML.
    // This property is backed by the 'minimum()' getter method and is marked as CONSTANT,
    // Meaning its value does not change during the lifetime of the object.
    Q_PROPERTY(float maximum READ maximum CONSTANT)

    // Expose the operation's name to QML.
    // This property is backed by the 'maximum()' getter method and is marked as CONSTANT.
    Q_PROPERTY(QString name READ name CONSTANT)

    // Expose the operation's activity state to QML.
    // This property is backed by the 'isActive()' getter method and emits 'isActiveChanged' when it changes.
    Q_PROPERTY(bool active READ isActive NOTIFY isActiveChanged)
private:
    /**
     * @brief Structure holding the specific parameters for the brightness operation.
     * Uses the RelativeAdjustmentParams which is suitable for adjustments in a range like [-1.0, 1.0].
    */
    Domain::RelativeAdjustmentParams m_params;

protected:
    ImageController* m_image_controller{nullptr};
public:
    /**
     * @brief Constructs a BrightnessModel.
     * @param parent Optional parent QObject.
     */
    explicit BrightnessModel(QObject* parent = nullptr);

    /**
     * @brief Gets the name of this operation.
     * @return QString "Brightness".
     */
    QString name() const override { return "Brightness"; }

    /**
     * @brief Checks if the brightness operation is currently active.
     * An operation is considered active if its internal parameters (m_params) indicate a change
     * from the default/no-op state (e.g., brightness value is not zero).
     * @return true if the operation is active, false otherwise.
     */
    [[no_discard]] bool isActive() const override { return m_params.isActive(); } 

    /**
     * @brief Gets the type of this operation.
     * @return OperationType::Brightness.
     */
    OperationType getType() const override { return OperationType::Brightness; }

    /**
     * @brief Creates and returns an OperationDescriptor representing the current state of this model.
     * This descriptor contains the operation type, name, parameters (like the brightness value),
     * and other metadata required by the core processing engine (PipelineEngine).
     * @return OperationDescriptor representing the current brightness state.
     */
    OperationDescriptor getDescriptor() const override;

    /**
     * @brief Sets the ImageController reference used for applying operations.
     * @param controller Pointer to the ImageController instance.
     */
    void setImageController(ImageController* controller) override;
    
    /**
     * @brief Resets the brightness value to its default (0.0).
     *
     * This slot resets the brightness to its neutral state (no adjustment).
     * It triggers an update via the ImageController.
     */
    void reset() override;

    /**
     * @brief Gets the current brightness value.
     * @return The current brightness value.
     */
    float value() const { return m_params.value; }
    
    /**
     * @brief Gets the minimum allowed brightness value.
     * @return float -1.0f, as defined by RelativeAdjustmentParams::MIN_VALUE.
     */
    constexpr float minimum() const { return Domain::RelativeAdjustmentParams::MIN_VALUE; }

    /**
     * @brief Gets the maximum allowed brightness value.
     * @return float 1.0f, as defined by RelativeAdjustmentParams::MAX_VALUE.
     */
    constexpr float maximum() const { return Domain::RelativeAdjustmentParams::MAX_VALUE; }

public slots:
    /**
     * @brief Sets the brightness value and triggers an update.
     *
     * This slot is intended to be called from QML when the user adjusts the brightness.
     * It validates the input, updates the internal RelativeAdjustmentParams structure,
     * and signals the ImageController to apply the new brightness value.
     *
     * @param value The new brightness value (clamped between minimum() and maximum()).
     */
    void setValue(float value);

    void onOperationCompleted() override;
    void onOperationFailed(const QString& error) override;
signals:

    /**
     * @brief Signal emitted when the brightness value changes.
     * QML can connect to this signal to react to value updates (e.g., refresh display).
     * ImageController should connect to this signal to trigger processing.
     * @param value The new value of the brightness parameter.
    */
    void valueChanged(float value); 
};

} // namespace CaptureMoment::UI
