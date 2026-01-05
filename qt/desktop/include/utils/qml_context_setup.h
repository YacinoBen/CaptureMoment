/**
 * @file qml_context_setup.h
 * @brief Manages creation and setup of all C++ objects for QML
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <QQmlContext>
#include <memory>

#include "controller/image_controller_base.h"
#include "models/manager/operation_model_manager.h"

// Forward declarations
namespace CaptureMoment::UI {

namespace Controller {
class ImageControllerBase;
}

namespace Models::Manager {
class OperationModelManager;
}

/**
     * @brief Central manager for QML context setup
     *
     * Responsibilities:
     * - Creates and manages core objects (controller, display item)
     * - Delegates operation model management to OperationModelManager
     * - Registers core objects to QML context
     * - Lifetime management via shared_ptr
     * - Ensures objects are correctly initialized before use
     */
class QmlContextSetup {
public:
    /**
     * @brief Setup complete QML context with all objects
     *
     * Creates:
     * - ImageController (main orchestrator)
     * - OperationModelManager (delegates model creation/connection/registration)
     *
     * Registers to QML:
     * - "controller" → ImageController
     * - Operation models are registered by OperationModelManager
     *
     * @param context The QML context to setup
     * @return true if setup was successful, false otherwise.
     */
    static bool setupContext(QQmlContext* context);

private:
    static std::shared_ptr<Controller::ImageControllerBase> m_controller;
    static std::unique_ptr<Models::Manager::OperationModelManager> m_operation_model_manager; // Nouveau membre

    /**
     * @brief Create the central ImageController orchestrator.
     * @return true if the controller was created successfully, false otherwise.
     */
    [[nodiscard]] static bool createController();

    /**
     * @brief Register core objects (like controller) to QML context
     */
    [[nodiscard]] static bool registerCoreToQml(QQmlContext* context);
};

} // namespace CaptureMoment::UI
