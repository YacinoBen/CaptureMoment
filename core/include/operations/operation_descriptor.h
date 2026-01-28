/**
 * @file operation_descriptor.h
 * @brief Descriptor of an operation with its parameters
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include "operation_type.h"
#include "common/error_handling/core_error.h"
#include <string>
#include <unordered_map>
#include <variant>
#include <expected>

namespace CaptureMoment::Core {

namespace Operations {

/**
 * @brief Supported types for operation parameters.
 * Using variant instead of std::any for compile-time type safety and performance.
 */
using OperationValue = std::variant<float, int, bool, std::string>;

/**
 * @struct OperationDescriptor
 * @brief A universal container for operation settings.
 */
struct OperationDescriptor {
    OperationType type;
    std::string name;
    bool enabled{true};

    /**
     * @brief Generic parameter storage using variant (Type Safe).
     */
    std::unordered_map<std::string, OperationValue> params;

    /**
     * @brief Helper to get a parameter value safely with error handling.
     * @tparam T The expected type (must be one of OperationValue types).
     * @param key The parameter name.
     * @return std::expected<T, CoreError> The value or an error (NotFound, InvalidType).
     */
    template <typename T>
    [[nodiscard]] std::expected<T, ErrorHandling::CoreError> getParam(const std::string& key) const
    {
        auto it = params.find(key);
        if (it == params.end()) {
            return std::unexpected(ErrorHandling::CoreError::Unexpected);
        }

        if (std::holds_alternative<T>(it->second)) {
            return std::get<T>(it->second);
        } else {
            return std::unexpected(ErrorHandling::CoreError::Unexpected);
        }
    }

    /**
     * @brief Helper to set a parameter value.
     * @tparam T The type of the value (must be one of OperationValue types).
     */
    template <typename T>
    void setParam(const std::string& key, const T& value) {
        params[key] = value;
    }
};

} // namespace Operations

} // namespace CaptureMoment::Core
