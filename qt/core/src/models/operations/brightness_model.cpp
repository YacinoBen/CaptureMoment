/**
 * @file brightness_model.cpp
 * @brief Implementation of BrightnessModel
 * @author CaptureMoment Team
 * @date 2025
 */

#include "models/operations/brightness_model.h" 
#include <spdlog/spdlog.h>
#include <algorithm> // For std::clamp

namespace CaptureMoment::UI {

    // Constructor: Initializes the model with a default value of 0.0.
    BrightnessModel::BrightnessModel(QObject* parent)
        : IOperationModel(parent),m_params{} 
    {
        // Initialize the parameter structure's value.
        m_params.value = 0.0f; // Default: no brightness adjustment
        spdlog::debug("BrightnessModel: Created with default value {}", m_params.value);
    }

    // Sets the ImageController reference used for applying operations.
    void BrightnessModel::setImageController(ImageController* controller) 
    {
        m_image_controller = controller; // Store the controller pointer

        if (m_image_controller) {
            spdlog::debug("BrightnessModel: ImageController set");

            // Connect to the controller's feedback signals to update this model's state or UI feedback.
            connect(m_image_controller, &ImageController::operationCompleted,
                    this, &BrightnessModel::onOperationApplied,
                    Qt::UniqueConnection); // Prevent duplicate connections

            connect(m_image_controller, &ImageController::operationFailed,
                    this, &BrightnessModel::onOperationFailed,
                    Qt::UniqueConnection); // Prevent duplicate connections

            // Register this model with the controller
            m_image_controller->registerModel(this); 
        } else {
            spdlog::warn("BrightnessModel: ImageController set to nullptr");
        }
    }

    // Sets the brightness value and triggers an update.
    void BrightnessModel::setValue(float value) 
    {
        // Clamp the incoming value to the allowed range [-1.0, 1.0]
        // defined by RelativeAdjustmentParams::MIN_VALUE and MAX_VALUE.
        float clampedValue = std::clamp(value, minimum(), maximum());

        // Check if the value has actually changed to avoid unnecessary updates/signals.
        if (qFuzzyCompare(m_params.value, clampedValue)) {
            return; // No change, exit early.
        }
        bool was_active = m_params.isActive();

        // Update the internal parameter structure.
        m_params.value = clampedValue;
        m_params.clampValue(); // Ensure it's within [-1, 1] using the struct's method

        spdlog::debug("BrightnessModel::setValue: New value = {}", m_params.value);

       bool is_active_now = m_params.isActive();
        // --- Create OperationDescriptor for PhotoEngine ---
        OperationDescriptor descriptor;
        descriptor.type = OperationType::Brightness; // Set the operation type
        descriptor.name = "Brightness (" + std::to_string(static_cast<int>(m_params.value * 100)) + "%)"; // Set a readable name
        descriptor.enabled = true; // Enable the operation
        descriptor.setParam<float>("value", m_params.value); // Set the specific parameter using the correct method

        spdlog::debug("BrightnessModel: Created descriptor - name='{}', value={}",
                      descriptor.name, m_params.value);


        // Emit the signal to notify QML that the value has changed.
        emit valueChanged(m_params.value);

        if (was_active != is_active_now) {
            emit isActiveChanged(); // Pass the new active state
            spdlog::debug("BrightnessModel::setValue: Activity state changed to {}", is_active_now);
        }

        // --- Trigger processing via ImageController ---
        if (m_image_controller) {
            // Send the single operation descriptor to the controller
            // applyOperations expects a vector
            std::vector<OperationDescriptor> operations = {descriptor};
            m_image_controller->applyOperations(operations); // Call the controller's method
        } else {
            spdlog::warn("BrightnessModel::setValue: No ImageController set, cannot apply operation.");
        }
    }

    // Resets the brightness value to its default (0.0).
    void BrightnessModel::reset() 
    {
        spdlog::debug("BrightnessModel::reset: Resetting to default value (0.0)");
        setValue(0.0f); // Call setValue to handle the update, clamping, signaling, and controller communication
    }

    // Handles successful operation completion from ImageController.
    void BrightnessModel::onOperationApplied() 
    {
        spdlog::debug("BrightnessModel: Operation applied successfully");
        // Potentially update internal state if needed, or just emit a signal for UI feedback
        emit operationApplied(); // Relay the signal to QML
    }

    // Handles operation failure from ImageController.
    void BrightnessModel::onOperationFailed(const QString& error)
    {
        spdlog::error("BrightnessModel: Operation failed - {}", error.toStdString());
        // Potentially update internal state if needed, or just emit a signal for UI feedback
        emit operationFailed(error); // Relay the signal to QML
    }

} // namespace CaptureMoment::UI