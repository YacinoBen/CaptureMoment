#pragma once

#include <memory>
#include <vector>
#include <QQmlContext>
#include <type_traits>

#include "models/operations/operation_provider.h"
#include "models/operations/base_adjustment_model.h"

#include <spdlog/spdlog.h>

namespace CaptureMoment::UI {

namespace Models::Manager {

/**
 * @brief Manages the creation, storage, and QML registration of operation model instances.
 *
 * This class is responsible for creating specific instances of operation models
 * (e.g., BrightnessModel, ContrastModel) derived from OperationProvider,
 * storing them internally, and registering them within the QML context
 * so they can be accessed from QML components.
 * It does not handle the connection of model signals to processing logic directly.
 */
class OperationModelManager {
public:
    /**
     * @brief Constructs an OperationModelManager.
     * Initializes the manager without requiring external dependencies like ImageControllerBase.
     */
    explicit OperationModelManager();

    /**
     * @brief Destructor.
     * Cleans up the owned operation model instances.
     */
    ~OperationModelManager() = default;

    // Disable copy and assignment
    OperationModelManager(const OperationModelManager&) = delete;
    OperationModelManager& operator=(const OperationModelManager&) = delete;

    /**
     * @brief Creates instances of basic adjustment operation models.
     * This method typically creates models like Brightness, Contrast, Highlights, etc.
     * @return true if all basic models were created successfully, false otherwise.
     */
    [[nodiscard]] bool createBasicAdjustmentModels();

    /**
     * @brief Creates instances of specified operation model types.
     * This is a variadic template method allowing for flexible model creation.
     * @tparam TModels Types of the operation models to create (must derive from OperationProvider).
     * @return true if all specified models were created successfully, false otherwise.
     */
    template<typename... TModels>
    [[nodiscard]] bool createModels();

    /**
     * @brief Registers the created operation models to the QML context.
     * Makes the models available under their generated names (e.g., brightnessControl) in QML.
     * @param context Pointer to the QQmlContext where models should be registered.
     * @return true if all models were registered successfully, false otherwise.
     */
    [[nodiscard]] bool registerModelsToQml(QQmlContext* context);

    /**
     * @brief Gets a const reference to the vector of created operation model instances.
     * Provides access to the internal list of models for external components
     * (like ImageControllerBase) to manage connections or retrieve state.
     * @return A const reference to the vector of shared_ptr<OperationProvider>.
     */
    [[nodiscard]] const std::vector<std::shared_ptr<Models::Operations::OperationProvider>>& getModels() const { return m_operation_models; }

    /**
     * @brief Gets a const reference to the vector of created BaseAdjustmentModel instances.
     * Provides access to the internal list of adjustment models for external components
     * (like ImageControllerBase) to manage connections or retrieve state efficiently
     * without needing dynamic casting.
     * @return A const reference to the vector of shared_ptr<BaseAdjustmentModel>.
     */
    [[nodiscard]] const std::vector<std::shared_ptr<UI::Models::Operations::BaseAdjustmentModel>>& getBaseAdjustmentModels() const { return m_base_adjustment_models; }

private:
    /**
     * @brief Internal storage for the created operation model instances.
     * Models are stored as shared_ptr to manage their lifetime safely.
     */
    std::vector<std::shared_ptr<Models::Operations::OperationProvider>> m_operation_models;

    /**
     * @brief Internal storage for the created BaseAdjustmentModel instances.
     * This list is populated alongside m_operation_models when models are created,
     * specifically for those models that derive from BaseAdjustmentModel.
     * It allows for efficient and type-safe access to adjustment models without casting.
     */
    std::vector<std::shared_ptr<UI::Models::Operations::BaseAdjustmentModel>> m_base_adjustment_models;

    /**
     * @brief Internal helper template function to create and add a single model instance.
     * Verifies the type T derives from OperationProvider at compile time.
     * @tparam T Type of the operation model to create.
     * @return true if the model was created and added successfully, false otherwise.
     */
    template<typename T>
    [[nodiscard]] bool createAndAddModel() {
        static_assert(std::is_base_of_v<UI::Models::Operations::OperationProvider, T>, "T must derive from OperationProvider");

        auto model = std::make_shared<T>();
        if (!model) {
            spdlog::error("OperationModelManager::createAndAddModel: Failed to create model of type {}", typeid(T).name());
            return false;
        }
        spdlog::debug("OperationModelManager::createAndAddModel: Created model of type {}", typeid(T).name());

        QString qmlName = model->name().toLower() + "Control";
        spdlog::debug("OperationModelManager::createAndAddModel: Generated QML name '{}'", qmlName.toStdString());

        m_operation_models.push_back(model);

        // If it's a BaseAdjustmentModel, add it to the specific list using dynamic_pointer_cast
        if constexpr (std::is_base_of_v<UI::Models::Operations::BaseAdjustmentModel, T>) {
            auto adjustment_model_ptr = std::dynamic_pointer_cast<UI::Models::Operations::BaseAdjustmentModel>(model);
            if (adjustment_model_ptr) {
                m_base_adjustment_models.push_back(adjustment_model_ptr);
            } else {
                // This should ideally not happen if the static_assert holds, but good for safety
                spdlog::warn("OperationModelManager::createAndAddModel: Failed dynamic_pointer_cast for type {}", typeid(T).name());
            }
        }

        return true;
    }
};

} // namespace Models::Manager

} // namespace CaptureMoment::UI
