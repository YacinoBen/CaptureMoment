/**
 * @file qml_context_setup.cpp
 * @brief Implementation of QmlContextSetup
 * @author CaptureMoment Team
 * @date 2025
 */

#include "utils/qml_context_setup.h"
#include "controller/image_controller_sgs.h"
#include "serializer/ui_serializer_manager.h"

#include <spdlog/spdlog.h>

namespace CaptureMoment::UI {

// Static member initialization
std::shared_ptr<Controller::ImageControllerBase> QmlContextSetup::m_controller_main_scene = nullptr;
std::unique_ptr<Serializer::UISerializerManager> QmlContextSetup::m_ui_serializer_manager = nullptr;

bool QmlContextSetup::setupContext(QQmlContext* context)
{
    if (!context) {
        spdlog::error("QmlContextSetup::setupContext: QML Context is null!");
        return false;
    }
    spdlog::info("QmlContextSetup: Starting QML context setup...");

    if (!createControllerMainScene()) {
        spdlog::error("QmlContextSetup::setupContext: Failed to create ImageController.");
        return false;
    }

    if (!m_controller_main_scene) {
        spdlog::error("QmlContextSetup::setupContext: m_controller_main_scene is null after createControllerMainScene!");
        return false;
    }

    // Check if UISerializerManager is set before attempting registration
    if (!m_ui_serializer_manager) {
        spdlog::warn("QmlContextSetup::setupContext: UISerializerManager is not set. Skipping registration.");
    }

    if (!registerCoreToQml(context)) {
        spdlog::error("QmlContextSetup::setupContext: Failed to register core objects to QML.");
        return false;
    }

    // Register UISerializerManager if it was provided
    if (m_ui_serializer_manager) {
        if (!registerUISerializerToQml(context)) {
            spdlog::error("QmlContextSetup::setupContext: Failed to register UISerializerManager to QML.");
            return false;
        }
    }

    // Get the Model Manager from the controller
    auto* op_model_manager = m_controller_main_scene->operationModelManager();
    if (!op_model_manager) {
        spdlog::error("QmlContextSetup::setupContext: OperationModelManager from ImageControllerBase is null.");
        return false;
    }

    // Register all operation models to QML via the manager
    if (!op_model_manager->registerModelsToQml(context)) {
        spdlog::error("QmlContextSetup::setupContext: Failed to register operation models to QML via OperationModelManager.");
        return false;
    }

    spdlog::info("QmlContextSetup: QML context setup completed successfully.");
    return true;
}

bool QmlContextSetup::createControllerMainScene()
{
    spdlog::debug("QmlContextSetup::createControllerMainScene: Creating ImageController...");

    m_controller_main_scene = std::make_shared<Controller::ImageControllerSGS>();

    if (!m_controller_main_scene) {
        spdlog::error("QmlContextSetup::createControllerMainScene: Failed to create ImageController (out of memory or constructor threw).");
        return false;
    }
    spdlog::debug("ImageControllerBase created.");

    return true;
}

bool QmlContextSetup::registerCoreToQml(QQmlContext* context)
{
    spdlog::debug("QmlContextSetup::registerCoreToQml: Registering objects to QML...");

    if (!context) {
        spdlog::error("QmlContextSetup::registerCoreToQml: QML Context is null!");
        return false;
    }

    if (!m_controller_main_scene) {
        spdlog::error("QmlContextSetup::registerCoreToQml: ImageController is null, cannot register.");
        return false;
    }

    context->setContextProperty("controller", m_controller_main_scene.get());
    spdlog::debug("Controller Base registered to QML context.");

    spdlog::info("QmlContextSetup::registerCoreToQml: Objects registered to QML successfully.");
    return true;
}

void QmlContextSetup::setUISerializerManager(std::unique_ptr<Serializer::UISerializerManager> ui_serializer_manager)
{
    m_ui_serializer_manager = std::move(ui_serializer_manager);
    spdlog::debug("QmlContextSetup::setUISerializerManager: UISerializerManager set.");
}

bool QmlContextSetup::registerUISerializerToQml(QQmlContext* context)
{
    spdlog::debug("QmlContextSetup::registerUISerializerToQml: Registering UISerializerManager to QML...");

    if (!context) {
        spdlog::error("QmlContextSetup::registerUISerializerToQml: QML Context is null!");
        return false;
    }

    if (!m_ui_serializer_manager) {
        spdlog::error("QmlContextSetup::registerUISerializerToQml: UISerializerManager is null, cannot register.");
        return false;
    }

    context->setContextProperty("uiSerializerManager", m_ui_serializer_manager.get());
    spdlog::debug("UISerializerManager registered to QML context.");

    spdlog::info("QmlContextSetup::registerUISerializerToQml: UISerializerManager registered to QML successfully.");
    return true;
}

} // namespace CaptureMoment::UI
