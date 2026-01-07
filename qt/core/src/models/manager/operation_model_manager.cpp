/**
 * @file operation_model_manager.cpp
 * @brief Implementation of OperationModelManager
 * @author CaptureMoment Team
 * @date 2025
 */

#include "models/manager/operation_model_manager.h"
#include "models/operations/basic_adjustment_models/brightness_model.h"
#include "models/operations/basic_adjustment_models/contrast_model.h"
#include "models/operations/basic_adjustment_models/highlights_model.h"
#include "models/operations/basic_adjustment_models/shadows_model.h"
#include "models/operations/basic_adjustment_models/whites_model.h"
#include "models/operations/basic_adjustment_models/blacks_model.h"

#include <spdlog/spdlog.h>

namespace CaptureMoment::UI::Models::Manager {

OperationModelManager::OperationModelManager()
{
    spdlog::debug("OperationModelManager: Constructed.");
}

bool OperationModelManager::createBasicAdjustmentModels()
{
    spdlog::info("OperationModelManager::createBasicAdjustmentModels: Creating basic adjustment models...");

    // Create instances of basic adjustment models
    if (!createAndAddModel<UI::Models::Operations::BrightnessModel>() ||
        !createAndAddModel<UI::Models::Operations::ContrastModel>() ||
        !createAndAddModel<UI::Models::Operations::HighlightsModel>() ||
        !createAndAddModel<UI::Models::Operations::ShadowsModel>() ||
        !createAndAddModel<UI::Models::Operations::WhitesModel>() ||
        !createAndAddModel<UI::Models::Operations::BlacksModel>())
    {
        spdlog::error("OperationModelManager::createBasicAdjustmentModels: Failed to create one or more basic adjustment models.");
        return false;
    }

    spdlog::info("OperationModelManager::createBasicAdjustmentModels: Successfully created {} basic adjustment models.", m_operation_models.size());
    return true;
}

bool OperationModelManager::registerModelsToQml(QQmlContext* context)
{
    if (!context) {
        spdlog::error("OperationModelManager::registerModelsToQml: QML Context is null!");
        return false;
    }

    spdlog::info("OperationModelManager::registerModelsToQml: Registering {} models to QML...", m_operation_models.size());

    for (const auto& model : m_operation_models) {
        if (!model) {
            spdlog::warn("OperationModelManager::registerModelsToQml: Found null model in list, skipping.");
            continue;
        }

        QString qml_name = model->name().toLower() + "Control";
        context->setContextProperty(qml_name, model.get());
        spdlog::debug("OperationModelManager::registerModelsToQml: Registered model '{}' as '{}' in QML.", model->name().toStdString(), qml_name.toStdString());
    }

    spdlog::info("OperationModelManager::registerModelsToQml: Successfully registered {} models to QML.", m_operation_models.size());
    return true;
}

} // namespace CaptureMoment::UI::Models::Manager
