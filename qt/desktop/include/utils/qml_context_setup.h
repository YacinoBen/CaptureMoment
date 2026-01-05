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

namespace Models::Operations {
class BrightnessModel;
class ContrastModel;
class HighlightsModel;
class ShadowsModel;
class WhitesModel;
}

namespace Controller {
class ImageControllerBase;
}

/**
     * @brief Central manager for QML context setup
     *
     * Responsibilities:
     * - Creates and manages all C++ objects (controller, models, display item)
     * - Connects models to controller and display item to controller
     * - Registers necessary objects to QML context
     * - Lifetime management via shared_ptr
     * - Ensures objects are correctly initialized before use
     *
     * QML doesn't know about internal models directly, it calls controller methods.
     * The display item (RhiImageItem) is also registered to QML for image rendering.
     */
class QmlContextSetup {
public:
    /**
     * @brief Setup complete QML context with all objects
     *
     * Creates:
     * - ImageController (main orchestrator)
     * - RHIImageItem (image display component)
     * - BrightnessModel (image adjustment model)
     * - All other operation models (TODO)
     *
     * Registers to QML:
     * - "controller" → ImageController
     * - "rhiImageItem" → RHIImageItem (for display)
     *
     * Models stay internal and are managed here
     *
     * @param context The QML context to setup
     * @return true if setup was successful, false otherwise.
     */
    static bool setupContext(QQmlContext* context);

private:
    // Shared pointers for lifetime management
    static std::shared_ptr<Controller::ImageControllerBase> m_controller;

    static std::shared_ptr<Models::Operations::BrightnessModel> m_brightness_model;
    static std::shared_ptr<Models::Operations::ContrastModel> m_contrast_model;
    static std::shared_ptr<Models::Operations::HighlightsModel> m_highlights_model;
    static std::shared_ptr<Models::Operations::ShadowsModel> m_shadows_model;
    static std::shared_ptr<Models::Operations::WhitesModel> m_whites_model;

    /**
     * @brief Create the central ImageController orchestrator.
     * @return true if the controller was created successfully, false otherwise.
     */
    [[nodiscard]] static bool createController();

    /**
     * @brief Create the operation model objects (e.g., BrightnessModel).
     * @return true if all models were created successfully, false otherwise.
     */
    [[nodiscard]] static bool createOperationModels();

    /**
     * @brief Connect models and display item to controller
     * @return true if all connections were successful, false otherwise.
     */
    [[nodiscard]] static bool connectObjects();

    /**
     * @brief Register necessary objects to QML context
     * @param context The QML context
     * @return true if registration was successful, false otherwise.
     */
    [[nodiscard]] static bool registerToQml(QQmlContext* context);

    /**
     * @brief Register necessary Models  to QML context
     * @param context The QML context
     * @return true if registration was successful, false otherwise.
     */
    [[nodiscard]] static bool registerModelsToQml(QQmlContext* context);
};

} // namespace CaptureMoment::UI
