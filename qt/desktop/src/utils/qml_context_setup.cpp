/**
 * @file qml_context_setup.cpp
 * @brief Implementation of QmlContextSetup
 * @author CaptureMoment Team
 * @date 2025
 */

#include "utils/qml_context_setup.h"
#include "image_controller.h"
#include "models/operations/brightness_model.h"

#include <spdlog/spdlog.h>

namespace CaptureMoment::UI {

// Static member initialization
std::shared_ptr<ImageController> QmlContextSetup::m_controller = nullptr;
std::shared_ptr<BrightnessModel> QmlContextSetup::m_brightnessModel = nullptr;

void QmlContextSetup::setupContext(QQmlContext* context) {
    if (!context) {
        spdlog::error("QmlContextSetup::setupContext: Invalid context");
        return;
    }
    
    spdlog::info("QmlContextSetup: Setting up QML context");
    
    // Step 1: Create all objects internally
    createObjects();
    
    // Step 2: Connect models to controller
    connectModels();
    
    // Step 3: Register only controller to QML
    registerToQml(context);
    
    spdlog::info("QmlContextSetup: QML context setup complete");
}

void QmlContextSetup::createObjects() {
    spdlog::debug("QmlContextSetup::createObjects: Creating all objects");
    
    // Create controller
    m_controller = std::make_shared<ImageController>();
    spdlog::debug("  ✓ ImageController created");
    
    // Create operation models
    m_brightnessModel = std::make_shared<BrightnessModel>();
    spdlog::debug("  ✓ BrightnessModel created");
    
    // TODO: Create more models as needed
    // m_contrastModel = std::make_shared<ContrastModel>();
    // m_saturationModel = std::make_shared<SaturationModel>();
}

void QmlContextSetup::connectModels() {
    spdlog::debug("QmlContextSetup::connectModels: Connecting models to controller");
    
    if (!m_controller || !m_brightnessModel) {
        spdlog::error("QmlContextSetup::connectModels: Objects not initialized");
        return;
    }
    
    // Connect brightness model to controller
    m_brightnessModel->setImageController(m_controller.get());
    spdlog::debug("  ✓ BrightnessModel connected");
    
    // TODO: Connect more models as they are created
    // m_contrastModel->setImageController(m_controller.get());
    // m_saturationModel->setImageController(m_controller.get());
}

void QmlContextSetup::registerToQml(QQmlContext* context) {
    spdlog::debug("QmlContextSetup::registerToQml: Registering objects");
    
    if (!m_controller) {
        spdlog::error("QmlContextSetup::registerToQml: Controller not initialized");
        return;
    }
    
    // Register ONLY controller to QML
    // Models are internal and managed here
    context->setContextProperty("controller", m_controller.get());
    spdlog::debug("  ✓ controller registered to QML");
}

} // namespace CaptureMoment::UI
