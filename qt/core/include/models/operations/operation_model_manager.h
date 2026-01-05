#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <QQmlContext>
#include <type_traits> // For std::void_t (C++17) or std::conjunction_v (C++17/20)

#include "models/operations/i_operation_model.h" // For IOperationModel
#include "controller/image_controller_base.h"    // For ImageControllerBase

namespace CaptureMoment::UI {

namespace Controller {
class ImageControllerBase;
}

namespace Models::Manager {

/**
     * @brief Manages the creation, connection, and registration of operation models.
     *
     * This class centralizes the logic related to operation models,
     * relieving QmlContextSetup of this responsibility.
     */
class OperationModelManager {
public:
    explicit OperationModelManager(Controller::ImageControllerBase* controller);
    ~OperationModelManager() = default;

    // Disable copy and assignment
    OperationModelManager(const OperationModelManager&) = delete;
    OperationModelManager& operator=(const OperationModelManager&) = delete;

    /**
         * @brief Creates all operation models.
         * Uses a variadic template to create multiple models in one call.
         * @tparam TModels Types of the models to create.
         * @return true if creation was successful, false otherwise.
         */
    template<typename... TModels>
    [[nodiscard]] bool createModels();

    /**
         * @brief Connects all operation models to the controller.
         * @return true if connection was successful, false otherwise.
         */
    [[nodiscard]] bool connectModels();

    /**
         * @brief Registers all operation models to the QML context.
         * @param context The QML context.
         * @return true if registration was successful, false otherwise.
         */
    [[nodiscard]] bool registerModelsToQml(QQmlContext* context);

private:
    Controller::ImageControllerBase* m_controller; // Reference to the controller
    std::vector<std::shared_ptr<Models::Operations::IOperationModel>> m_operation_models; // Container for all models

    // Internal method to create a specific model and add it to the container
    template<typename T>
    [[nodiscard]] bool addModel(const QString& qmlName); // qmlName = "brightnessControl", "contrastControl", etc.

    // Helper to check if T has a 'name()' method returning QString
    template<typename T>
    using has_name_method = std::is_same_v<decltype(std::declval<T>().name()), QString>;

    // Internal helper to create a single model and add it to the container
    template<typename T>
    [[nodiscard]] bool createAndAddModel(const QString& qmlName);
};

} // namespace Models::Manager

} // namespace CaptureMoment::UI
