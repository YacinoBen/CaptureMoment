/**
 * @file operation_state_manager.cpp
 * @brief Implementation of OperationStateManager
 * @author CaptureMoment Team
 * @date 2025
 */

#include "managers/qoperation_state_manager.h"
#include "controller/image_controller_base.h"
#include "engine/photo_engine.h"
#include "operations/operation_descriptor.h"
#include <spdlog/spdlog.h>
#include <algorithm> // For std::find_if, std::remove_if, std::end, std::begin

namespace CaptureMoment::UI::Managers {

OperationStateManager::OperationStateManager(
    Controller::ImageControllerBase* controller,
    std::shared_ptr<Core::Engine::PhotoEngine> engine
    )
    : m_controller(controller)
    , m_engine(std::move(engine))
{
    if (!m_controller || !m_engine) {
        spdlog::critical("OperationStateManager: Null dependency provided during construction.");
        throw std::invalid_argument("OperationStateManager: Null controller or engine provided.");
    }
    spdlog::debug("OperationStateManager: Constructed.");
}

void OperationStateManager::addOrUpdateOperation(const Core::Operations::OperationDescriptor& descriptor) {
    std::lock_guard lock(m_mutex);
    spdlog::debug("OperationStateManager::addOrUpdateOperation: Adding/updating operation '{}'.", descriptor.name);

    // Find if an operation of the same type already exists
    auto it = std::find_if(m_active_operations.begin(), m_active_operations.end(),
                           [&descriptor](const auto& op) { return op.type == descriptor.type; });

    if (it != m_active_operations.end()) {
        // Update existing operation
        *it = descriptor;
        spdlog::debug("OperationStateManager::addOrUpdateOperation: Updated operation '{}'.", descriptor.name);
    } else {
        // Add new operation
        m_active_operations.push_back(descriptor);
        spdlog::debug("OperationStateManager::addOrUpdateOperation: Added new operation '{}'.", descriptor.name);
    }

    applyCurrentOperationsToEngine();
}

void OperationStateManager::removeOperation(Core::Operations::OperationType type) {
    std::lock_guard lock(m_mutex);
    spdlog::debug("OperationStateManager::removeOperation: Removing operation type '{}'.", static_cast<int>(type));

    // Remove operation of the specified type
    m_active_operations.erase(
        std::remove_if(m_active_operations.begin(), m_active_operations.end(),
                       [type](const auto& op) { return op.type == type; }),
        m_active_operations.end()
        );

    spdlog::debug("OperationStateManager::removeOperation: Operation type '{}' removed (if it existed).", static_cast<int>(type));
    applyCurrentOperationsToEngine();
}

void OperationStateManager::clearAllOperations() {
    std::lock_guard lock(m_mutex);
    spdlog::debug("OperationStateManager::clearAllOperations: Clearing all operations.");

    m_active_operations.clear();
    spdlog::debug("OperationStateManager::clearAllOperations: All operations cleared.");
    applyCurrentOperationsToEngine();
}

std::vector<Core::Operations::OperationDescriptor> OperationStateManager::getActiveOperations() const {
    std::lock_guard lock(m_mutex);
    // Return a copy of the vector, safe for concurrent access by the caller
    return m_active_operations;
}

void OperationStateManager::applyCurrentOperationsToEngine() {
    // Get a copy of the current operations list to pass to the engine
    std::vector<Core::Operations::OperationDescriptor> ops_to_apply = getActiveOperations();

    spdlog::debug("OperationStateManager::applyCurrentOperationsToEngine: Applying {} operations to PhotoEngine.", ops_to_apply.size());

    // Delegate the application to the PhotoEngine
    m_engine->applyOperations(ops_to_apply);

    spdlog::debug("OperationStateManager::applyCurrentOperationsToEngine: Operations sent to PhotoEngine for update.");
}

} // namespace CaptureMoment::UI::Managers
