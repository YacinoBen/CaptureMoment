/**
 * @file qml_context_setup.cpp
 * @brief Implementation of QmlContextSetup
 * @author CaptureMoment Team
 * @date 2025
 */

#include "utils/qml_context_setup.h"
#include "models/operations/brightness_model.h"
#include "controller/image_controller_painted.h"
#include "controller/image_controller_sgs.h"
#include "controller/image_controller_rhi.h"

#include <spdlog/spdlog.h>

namespace CaptureMoment::UI {

// Static member initialization (pointers are initialized to nullptr by default)
std::shared_ptr<Controller::ImageControllerBase> QmlContextSetup::m_controller = nullptr;
std::shared_ptr<BrightnessModel> QmlContextSetup::m_brightness_model = nullptr;

bool QmlContextSetup::setupContext(QQmlContext* context)
{
    if (!context) {
        spdlog::error("QmlContextSetup::setupContext: QML Context is null!");
        return false; 
    }
    spdlog::info("QmlContextSetup: Starting QML context setup...");

    // Step 1: Create all objects (separated into logical groups)
    if (!createController()) {
        spdlog::error("QmlContextSetup::setupContext: Failed to create ImageController.");
        return false;
    }

    if (!createOperationModels()) {
        spdlog::error("QmlContextSetup::setupContext: Failed to create operation models.");
        return false; 
    }

    // Step 2: Connect objects together
    if (!connectObjects()) {
        spdlog::error("QmlContextSetup::setupContext: Failed to connect objects.");
        return false; 
    }

    // Step 3: Register necessary objects to QML context
    if (!registerToQml(context)) {
        spdlog::error("QmlContextSetup::setupContext: Failed to register objects to QML.");
        return false;
    }
    // Register Models
    if (!registerModelsToQml(context)) {
        spdlog::error("QmlContextSetup::setupContext: Failed to register objects to QML.");
        return false; 
    }

    spdlog::info("QmlContextSetup: QML context setup completed successfully.");
    return true;
}

// Creates the central ImageController orchestrator.
bool QmlContextSetup::createController()
{
    spdlog::debug("QmlContextSetup::createController: Creating ImageController...");

    m_controller = std::make_shared<Controller::ImageControllerSGS>();
    
    if (!m_controller) {
        spdlog::error("QmlContextSetup::createController: Failed to create ImageController (out of memory or constructor threw).");
        return false;
    }
    spdlog::debug("ImageControllerBase created.");

    return true;
}


// Creates the operation model objects (e.g., BrightnessModel).
bool QmlContextSetup::createOperationModels()
{
    spdlog::debug("QmlContextSetup::createOperationModels: Creating operation models...");

    m_brightness_model = std::make_shared<BrightnessModel>();
    if (!m_brightness_model) {
        spdlog::error("QmlContextSetup::createOperationModels: Failed to create BrightnessModel (out of memory or constructor threw).");
        return false;
    }
        spdlog::debug("  ✓ BrightnessModel created.");

    // TODO: Create more models as needed (e.g., ContrastModel, SaturationModel)
    // m_contrast_model = std::make_shared<ContrastModel>();
    // if (!m_contrast_model) { ... return false; }
    // spdlog::debug("  ✓ ContrastModel created.");

    return true; // Succès
}

// Connects models and display item to controller
bool QmlContextSetup::connectObjects()
{
    spdlog::debug("QmlContextSetup::connectObjects: Connecting objects...");

    if (!m_controller) {
        spdlog::error("QmlContextSetup::connectObjects: ImageController is null, cannot connect.");
        return false;
    }

    if (!m_brightness_model) {
        spdlog::error("QmlContextSetup::connectObjects: BrightnessModel is null, cannot connect.");
        return false; 
    }

    // --- Connect Operation Models to Controller ---
    // BrightnessModel needs to know the controller to communicate changes or register itself

    m_brightness_model->setImageController(m_controller.get());
    spdlog::debug("BrightnessModel connected to ImageController.");

    // TODO: Connect more models as they are created
    // m_contrast_model->setImageController(m_controller.get());
    // spdlog::debug("  ✓ ContrastModel connected to ImageController.");

    spdlog::info("QmlContextSetup::connectObjects: All objects connected successfully.");
    return true;
}

// Registers necessary objects to QML context
bool QmlContextSetup::registerToQml(QQmlContext* context)
{
    spdlog::debug("QmlContextSetup::registerToQml: Registering objects to QML...");

    if (!context) {
        spdlog::error("QmlContextSetup::registerToQml: QML Context is null!");
        return false;
    }

    if (!m_controller) {
        spdlog::error("QmlContextSetup::registerToQml: ImageController is null, cannot register.");
        return false; 
    }

    // --- Register Controller ---
    context->setContextProperty("controller", m_controller.get());
    spdlog::debug("Controller Base registered to QML context.");


    // Note: The internal models (like m_brightness_model) are NOT registered here.
    // They are managed by ImageController.

    spdlog::info("QmlContextSetup::registerToQml: Objects registered to QML successfully.");
    return true;
}

bool QmlContextSetup::registerModelsToQml(QQmlContext* context)
{
    spdlog::debug("QmlContextSetup::registerModelsToQml: Registering objects to QML...");

    if (m_brightness_model) {
        context->setContextProperty("brightnessControl", m_brightness_model.get());
        spdlog::debug("'brightnessControl' registered to QML context.");
     } else {
            spdlog::warn("QmlContextSetup::registerToQml: BrightnessControl is null, skipping registration.");
    }

    // Note: The internal models (like m_brightness_model) are NOT registered here.
    // They are managed by ImageController.

    spdlog::info("QmlContextSetup::registerModelsToQml: Objects registered to QML successfully.");
    return true; // Succès
}

} // namespace CaptureMoment::UI
