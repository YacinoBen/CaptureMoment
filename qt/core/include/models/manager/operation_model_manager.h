#pragma once

#include <memory>
#include <vector>
#include <QQmlContext>
#include <type_traits>

#include "models/operations/operation_provider.h"
#include "controller/image_controller_base.h"

#include <spdlog/spdlog.h>

namespace CaptureMoment::UI {

namespace Controller {
class ImageControllerBase;
}

namespace Models::Manager {

class OperationModelManager {
public:
    explicit OperationModelManager(Controller::ImageControllerBase* controller);
    ~OperationModelManager() = default;

    OperationModelManager(const OperationModelManager&) = delete;
    OperationModelManager& operator=(const OperationModelManager&) = delete;

    [[nodiscard]] bool createBasicAdjustmentModels();

    template<typename... TModels>
    [[nodiscard]] bool createModels();

    [[nodiscard]] bool connectModels();

    [[nodiscard]] bool registerModelsToQml(QQmlContext* context);

private:
    Controller::ImageControllerBase* m_controller;
    std::vector<std::shared_ptr<Models::Operations::OperationProvider>> m_operation_models;

    template<typename T>
    [[nodiscard]] bool createAndAddModel() {
        static_assert(std::is_base_of_v<Models::Operations::OperationProvider, T>, "T must derive from OperationProvider");

        auto model = std::make_shared<T>();
        if (!model) {
            spdlog::error("OperationModelManager::createAndAddModel: Failed to create model of type {}", typeid(T).name());
            return false;
        }
        spdlog::debug("OperationModelManager::createAndAddModel: Created model of type {}", typeid(T).name());

        QString qmlName = model->name().toLower() + "Control";
        spdlog::debug("OperationModelManager::createAndAddModel: Generated QML name '{}'", qmlName.toStdString());

        m_operation_models.push_back(model);

        return true;
    }
};

} // namespace Models::Manager

} // namespace CaptureMoment::UI
