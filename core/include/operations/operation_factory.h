/**
 * @file operation_factory.h
 * @brief Factory for creating operations (Dependency Injection)
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include "operations/operation_type.h"
#include <memory>
#include <unordered_map>
#include <functional>

namespace CaptureMoment::Core {

namespace Operations {

// Forward declarations
class IOperation;
struct OperationDescriptor;

/**
 * @class OperationFactory
 * @brief Factory Pattern - creates operations from OperationDescriptor
 *
 * Single responsibility: mapping OperationType to concrete implementations.
 * No switch statements - uses function pointers for creation.
 */
class OperationFactory {
public:
    /**
     * @brief Register an operation creator for a given type using a template
     * @tparam T Concrete operation class (must inherit from Operation)
     * @param type OperationType identifier
     */
    template <typename T>
    void registerOperation(OperationType type) {
        m_creators[type] = []() -> std::unique_ptr<IOperation> {
            return std::make_unique<T>();
        };
    }

    /**
     * @brief Register an operation creator for a given type using a lambda/function
     * @param type OperationType identifier
     * @param creator Function/lambda that creates the operation instance
     */
    void registerCreator(OperationType type, std::function<std::unique_ptr<IOperation>()> creator) {
        m_creators[type] = std::move(creator);
    }

    /**
     * @brief Create an operation instance from descriptor
     * @param descriptor Operation descriptor with type information
     * @return Unique pointer to operation, or nullptr if type not registered
     */
    [[nodiscard]] std::unique_ptr<IOperation> create(const OperationDescriptor& descriptor) const;

private:
    using OperationCreator = std::function<std::unique_ptr<IOperation>()>;
    std::unordered_map<OperationType, OperationCreator> m_creators;
};

} // namespace Operations

} // namespace CaptureMoment::Core
