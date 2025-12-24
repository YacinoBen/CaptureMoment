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
        : OperationProvider(parent),m_params{} 
    {
        // Initialize the parameter structure's value.
        m_params.value = 0.0f; // Default: no brightness adjustment
        spdlog::debug("BrightnessModel: Created with default value {}", m_params.value);
    }
    
    Core::Operations::OperationDescriptor BrightnessModel::getDescriptor() const
    {
        Core::Operations::OperationDescriptor descriptor;
        descriptor.type = Core::Operations::OperationType::Brightness;
        descriptor.name = "Brightness (" + std::to_string(static_cast<int>(m_params.value * 100)) + "%)";
        descriptor.enabled = true;
        descriptor.setParam<float>("value", m_params.value);
        return descriptor;
    }
    // Sets the ImageControllerBase reference used for applying operations.
    void BrightnessModel::setImageController(Controller::ImageControllerBase* controller) 
    {
        m_image_controller = controller;
        if (!m_image_controller) 
        {
            spdlog::warn("BrightnessModel: ImageControllerBase set to nullptr");
            return;
        }
        spdlog::debug("BrightnessModel: ImageControllerBase set");
        
        // Register this model with the controller
        m_image_controller->registerModel(this);
        
        // Connect to controller's feedback signal
        connect(m_image_controller, &Controller::ImageControllerBase::operationCompleted,
            this, &BrightnessModel::onOperationCompleted);

        connect(m_image_controller, &Controller::ImageControllerBase::operationFailed,
            this, &BrightnessModel::onOperationFailed);
            spdlog::debug("BrightnessModel: Connected to ImageControllerBase signals");
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
        bool was_active  = m_params.isActive();

        // Update the internal parameter structure.
        m_params.value = clampedValue;
        m_params.clampValue(); // Ensure it's within [-1, 1] using the struct's method
        
        bool is_now_active = isActive();
        emit valueChanged(m_params.value);

        if (was_active != is_now_active) {
            emit isActiveChanged(); // Pass the new active state
            spdlog::debug("BrightnessModel::setValue: Activity state changed to {}", is_now_active);
        }
        Core::Operations::OperationDescriptor descriptor = getDescriptor();


        // --- Trigger processing via ImageControllerBase ---
        if (m_image_controller) {
            // Send the single operation descriptor to the controller
            // applyOperations expects a vector
            std::vector<Core::Operations::OperationDescriptor> operations = {descriptor};
            m_image_controller->applyOperations(operations); // Call the controller's method
        } else {
            spdlog::warn("BrightnessModel::setValue: No ImageControllerBase set, cannot apply operation.");
        }
    }

    // Resets the brightness value to its default (0.0).
    void BrightnessModel::reset() 
    {
        spdlog::debug("BrightnessModel::reset: Resetting to default value (0.0)");
        setValue(0.0f); // Call setValue to handle the update, clamping, signaling, and controller communication
    }

    // Handles successful operation completion from ImageControllerBase.
    void BrightnessModel::onOperationCompleted() 
    {
        spdlog::debug("BrightnessModel: Operation Completed successfully");
        // Potentially update internal state if needed, or just emit a signal for UI feedback
        emit operationApplied(); // Relay the signal to QML
    }

    // Handles operation failure from ImageControllerBase.
    void BrightnessModel::onOperationFailed(const QString& error)
    {
        spdlog::error("BrightnessModel: Operation failed - {}", error.toStdString());
        // Potentially update internal state if needed, or just emit a signal for UI feedback
        emit operationFailed(error); // Relay the signal to QML
    }

} // namespace CaptureMoment::UI
