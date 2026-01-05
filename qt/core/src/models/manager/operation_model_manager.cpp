#include "models/manager/operation_model_manager.h"

#include "controller/image_controller_base.h"
#include "models/operations/operation_provider.h"

#include "models/operations/basic_adjustment_models/brightness_model.h"
#include "models/operations/basic_adjustment_models/contrast_model.h"
#include "models/operations/basic_adjustment_models/highlights_model.h"
#include "models/operations/basic_adjustment_models/shadows_model.h"
#include "models/operations/basic_adjustment_models/whites_model.h"
#include "models/operations/basic_adjustment_models/blacks_model.h"

#include <spdlog/spdlog.h>

namespace CaptureMoment::UI::Models::Manager {

OperationModelManager::OperationModelManager(Controller::ImageControllerBase* controller)
    : m_controller(controller)
{
    if (!m_controller) {
        spdlog::error("OperationModelManager: Controller is null!");
    }
}

bool OperationModelManager::createBasicAdjustmentModels()
{
    spdlog::debug("OperationModelManager::createBasicAdjustmentModels: Creating basic adjustment models...");

    // Appelle la méthode template avec les types spécifiques
    return createModels<
        Models::Operations::BrightnessModel,
        Models::Operations::ContrastModel,
        Models::Operations::HighlightsModel,
        Models::Operations::ShadowsModel,
        Models::Operations::WhitesModel,
        Models::Operations::BlacksModel
    >();
}

template<typename... TModels>
bool OperationModelManager::createModels()
{
    spdlog::debug("OperationModelManager::createModels: Creating operation models...");

    bool success = (createAndAddModel<TModels>() && ...);

    if (success) {
        spdlog::info("OperationModelManager::createModels: {} models created.", sizeof...(TModels));
    } else {
        spdlog::error("OperationModelManager::createModels: Failed to create one or more models.");
    }
    return success;
}

template bool OperationModelManager::createModels<
    Models::Operations::BrightnessModel,
    Models::Operations::ContrastModel,
    Models::Operations::HighlightsModel,
    Models::Operations::ShadowsModel,
    Models::Operations::WhitesModel,
    Models::Operations::BlacksModel
>();

bool OperationModelManager::connectModels()
{
    spdlog::debug("OperationModelManager::connectModels: Connecting models to controller...");

    for (auto& model : m_operation_models) {
        if (!model) {
            spdlog::error("OperationModelManager::connectModels: Found a null model in the list.");
            return false;
        }

        model->setImageController(m_controller);
        spdlog::debug("OperationModelManager::connectModels: Model connected to controller.");
    }

    spdlog::info("OperationModelManager::connectModels: All models connected to controller.");
    return true;
}

bool OperationModelManager::registerModelsToQml(QQmlContext* context)
{
    spdlog::debug("OperationModelManager::registerModelsToQml: Registering models to QML...");

    if (!context) {
        spdlog::error("OperationModelManager::registerModelsToQml: QML Context is null!");
        return false;
    }

    for (auto& model : m_operation_models) {
        if (!model) {
            spdlog::warn("OperationModelManager::registerModelsToQml: Found a null model, skipping registration.");
            continue;
        }

        QString qmlName = model->name().toLower();
        context->setContextProperty(qmlName + "Control", model.get());
        spdlog::debug("OperationModelManager::registerModelsToQml: Registered '{}' to QML context.", qmlName.toStdString() + "Control");
    }

    spdlog::info("OperationModelManager::registerModelsToQml: All models registered to QML.");
    return true;
}

} // namespace CaptureMoment::UI::Models::Manager
