/**
 * @file operation_state_manager.h
 * @brief Manages the state of active image operations.
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include "operations/operation_descriptor.h"
#include "engine/photo_engine.h"
#include <vector>
#include <memory>
#include <mutex>
#include <string_view>

#include <spdlog/spdlog.h>

namespace CaptureMoment::UI {

namespace Controller {
class ImageControllerBase; // Forward declaration
}

namespace Managers {

/**
 * @brief Manages the state of active image operations.
 *
 * This class is responsible for maintaining the list of currently active operations
 * (e.g., brightness +0.5, contrast -0.2) and coordinating their application
 * to the PhotoEngine. It acts as an intermediary between the UI layer
 * (ImageControllerBase, OperationModels) and the core processing engine (PhotoEngine).
 * It ensures that the cumulative effect of all active operations is applied correctly.
 */
class OperationStateManager {
public:
    /**
     * @brief Constructs an OperationStateManager.
     * @param controller Pointer to the ImageControllerBase instance.
     * @param engine Shared pointer to the PhotoEngine instance.
     */
    explicit OperationStateManager(
        Controller::ImageControllerBase* controller,
        std::shared_ptr<Core::Engine::PhotoEngine> engine
        );

    ~OperationStateManager() = default;

    // Disable copy and assignment
    OperationStateManager(const OperationStateManager&) = delete;
    OperationStateManager& operator=(const OperationStateManager&) = delete;

    /**
     * @brief Adds or updates an operation in the active state.
     *
     * If an operation of the same type already exists, its parameters are updated.
     * Otherwise, a new operation is added to the list.
     * This method triggers an update of the working image via PhotoEngine.
     *
     * @param descriptor The operation descriptor to add or update.
     */
    void addOrUpdateOperation(const Core::Operations::OperationDescriptor& descriptor);

    /**
     * @brief Removes an operation of a specific type from the active state.
     *
     * If no operation of the given type exists, this method does nothing.
     * This method triggers an update of the working image via PhotoEngine.
     *
     * @param type The type of the operation to remove.
     */
    void removeOperation(Core::Operations::OperationType type);

    /**
     * @brief Clears all active operations, resetting the state to the original image.
     *
     * This method triggers an update of the working image via PhotoEngine.
     */
    void clearAllOperations();

    /**
     * @brief Gets the current list of active operation descriptors.
     *
     * This method is thread-safe.
     * @return A copy of the vector containing the active operation descriptors.
     */
    [[nodiscard]] std::vector<Core::Operations::OperationDescriptor> getActiveOperations() const;

private:
    mutable std::mutex m_mutex; //!< Mutex protecting access to m_active_operations.
    std::vector<Core::Operations::OperationDescriptor> m_active_operations; //!< The current list of active operations.

    Controller::ImageControllerBase* m_controller; //!< Pointer to the controller (for potential notifications or coordination).
    std::shared_ptr<Core::Engine::PhotoEngine> m_engine; //!< Shared pointer to the core processing engine.

    /**
     * @brief Internal method to apply the current list of active operations to the PhotoEngine.
     * This method is called after m_active_operations is modified.
     */
    void applyCurrentOperationsToEngine();
};

} // namespace Managers
} // namespace CaptureMoment::UI
