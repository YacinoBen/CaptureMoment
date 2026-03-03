/**
 * @file operation_descriptor.h
 * @brief Descriptor of an operation with its parameters and unique identifier.
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include "operations/operation_type.h"
#include "common/error_handling/core_error.h"

#include <string>
#include <unordered_map>
#include <variant>
#include <expected>
#include <cstdint>
#include <atomic>

namespace CaptureMoment::Core {

namespace Operations {

/**
 * @brief Supported types for operation parameters.
 * Using variant instead of std::any for compile-time type safety and performance.
 */
using OperationValue = std::variant<float, int, bool, std::string>;

/**
 * @struct OperationDescriptor
 * @brief A universal container for operation settings with unique identifier.
 *
 * @details
 * Each operation has a unique `id` that remains stable throughout its lifetime,
 * even when parameter values change. This enables efficient pipeline updates
 * without recompilation when only values change.
 *
 * The `id` is used as the cache key for Halide parameters, while `type`
 * determines the operation kind for factory creation.
 */
struct OperationDescriptor {
    /**
     * @brief Unique identifier for this operation instance.
     *
     * @details
     * Generated atomically and guaranteed unique within the application.
     * Used as a stable key for parameter caching, allowing the pipeline
     * to update values without recompilation.
     *
     * - Stable: Never changes after creation
     * - Unique: No two operations share the same id
     * - Fast: uint64_t comparison is O(1)
     */
    uint64_t id{0};

    /**
     * @brief The operation type (e.g., Brightness, Contrast, Saturation).
     * Used by the factory to create the appropriate operation implementation.
     */
    OperationType type;

    /**
     * @brief Human-readable name for display purposes.
     * May include parameter values, e.g., "Brightness (100%)".
     */
    std::string name;

    /**
     * @brief Whether this operation is currently active.
     * Disabled operations are skipped during pipeline execution.
     */
    bool enabled{true};

    /**
     * @brief Generic parameter storage using variant (Type Safe).
     */
    std::unordered_map<std::string, OperationValue> params;

    /**
     * @brief Generates a unique identifier for a new operation.
     * @return A unique uint64_t id.
     *
     * @par Thread Safety:
     * This method is thread-safe and uses atomic operations.
     */
    static uint64_t generateId() {
        static std::atomic<uint64_t> counter{1};
        return counter.fetch_add(1, std::memory_order_relaxed);
    }

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
