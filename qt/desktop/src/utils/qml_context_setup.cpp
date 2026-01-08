/**
 * @file qml_context_setup.cpp
 * @brief Implementation of QmlContextSetup
 * @author CaptureMoment Team
 * @date 2025
 */

#include "utils/qml_context_setup.h"
#include "controller/image_controller_sgs.h"

#include <spdlog/spdlog.h>

namespace CaptureMoment::UI {

// Static member initialization
std::shared_ptr<Controller::ImageControllerBase> QmlContextSetup::m_controller = nullptr;

bool QmlContextSetup::setupContext(QQmlContext* context)
{
    if (!context) {
        spdlog::error("QmlContextSetup::setupContext: QML Context is null!");
        return false;
    }
    spdlog::info("QmlContextSetup: Starting QML context setup...");

    if (!createController()) {
        spdlog::error("QmlContextSetup::setupContext: Failed to create ImageController.");
        return false;
    }

    if (!m_controller) {
        spdlog::error("QmlContextSetup::setupContext: m_controller is null after createController!");
        return false;
    }

    if (!registerCoreToQml(context)) {
        spdlog::error("QmlContextSetup::setupContext: Failed to register core objects to QML.");
        return false;
    }

    // Get the Model
    auto* op_model_manager = m_controller->operationModelManager();
    if (!op_model_manager) {
        spdlog::error("QmlContextSetup::setupContext: OperationModelManager from ImageControllerBase is null.");
        return false;
    }

    // For regist all operations
    if (!op_model_manager->registerModelsToQml(context)) {
        spdlog::error("QmlContextSetup::setupContext: Failed to register operation models to QML via OperationModelManager.");
        return false;
    }

    spdlog::info("QmlContextSetup: QML context setup completed successfully.");
    return true;
}

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

bool QmlContextSetup::registerCoreToQml(QQmlContext* context)
{
    spdlog::debug("QmlContextSetup::registerCoreToQml: Registering objects to QML...");

    if (!context) {
        spdlog::error("QmlContextSetup::registerCoreToQml: QML Context is null!");
        return false;
    }

    if (!m_controller) {
        spdlog::error("QmlContextSetup::registerCoreToQml: ImageController is null, cannot register.");
        return false;
    }

    context->setContextProperty("controller", m_controller.get());
    spdlog::debug("Controller Base registered to QML context.");

    spdlog::info("QmlContextSetup::registerCoreToQml: Objects registered to QML successfully.");
    return true;
}

} // namespace CaptureMoment::UI
