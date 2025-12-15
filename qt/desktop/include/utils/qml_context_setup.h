/**
 * @file qml_context_setup.h
 * @brief Manages creation and setup of all C++ objects for QML
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <QQmlContext>
#include <memory>

#include "image_controller.h"
#include "models/operations/brightness_model.h"

namespace CaptureMoment::UI {

// Forward declarations
class ImageController;
class BrightnessModel;
class IOperationModel;

/**
 * @brief Central manager for QML context setup
 * 
 * Responsibilities:
 * - Creates and manages all C++ objects (controller, models)
 * - Connects models to controller
 * - Registers objects to QML context
 * - Lifetime management via shared_ptr
 * 
 * QML doesn't know about models, it only calls controller methods
 */
class QmlContextSetup {
public:
    /**
     * @brief Setup complete QML context with all objects
     * 
     * Creates:
     * - ImageController (main orchestrator)
     * - BrightnessModel (image adjustment model)
     * - All other operation models
     * 
     * Registers to QML:
     * - "controller" → ImageController
     * 
     * Models stay internal and are managed here
     * 
     * @param context The QML context to setup
     */
    static void setupContext(QQmlContext* context);

private:
    // Shared pointers for lifetime management
    static std::shared_ptr<ImageController> m_controller;
    static std::shared_ptr<BrightnessModel> m_brightnessModel;
    
    /**
     * @brief Create all C++ objects
     */
    static void createObjects();
    
    /**
     * @brief Connect models to controller
     */
    static void connectModels();
    
    /**
     * @brief Register controller to QML
     * @param context The QML context
     */
    static void registerToQml(QQmlContext* context);
};

} // namespace CaptureMoment::UI

