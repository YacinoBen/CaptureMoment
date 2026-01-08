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

// Forward declarations
namespace CaptureMoment::UI {

namespace Controller {
class ImageControllerBase;
}

/**
 * @brief Central manager for QML context setup
 *
 * Responsibilities:
 * - Creates and manages core objects (controller)
 * - Delegates operation model management (creation, registration, connection) to ImageControllerBase
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
     *
     * Registers to QML:
     * - "controller" → ImageController (which internally manages models)
     *
     * @param context The QML context to setup
     * @return true if setup was successful, false otherwise.
     */
    static bool setupContext(QQmlContext* context);

private:

    /**
     * @brief Static shared pointer to the main ImageControllerBase instance.
     * This singleton-like pattern holds the central controller object which orchestrates
     * the image processing pipeline, manages operation models, and handles communication
     * between the UI layer and the core processing engine.
     * It is created by createController() and registered to the QML context.
     */
    static std::shared_ptr<Controller::ImageControllerBase> m_controller;

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
