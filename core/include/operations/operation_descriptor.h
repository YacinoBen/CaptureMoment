/**
 * @file operation_descriptor.h
 * @brief Descriptor of an operation with its parameters
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once
#include "operation_type.h"
#include <string>
#include <unordered_map>
#include <any>

namespace CaptureMoment::Core {

namespace Operations {

/**
 * @struct OperationDescriptor
 * @brief A universal container for operation settings.
 * * This structure holds all the information required to execute a specific
 * image processing operation. It uses a generic parameter map to support
 * any type of configuration (simple floats, complex objects, strings, etc.)
 * without changing the data structure.
 * * @par Usage Example
 * @code
 * OperationDescriptor desc;
 * desc.type = OperationType::Brightness;
 * desc.name = "Brightness";
 * desc.params["value"] = 0.2f; // Increase brightness by 0.2
 * * // Passing to execution
 * brightnessOp.execute(region, desc);
 * @endcode
 */

struct OperationDescriptor {
    /**
     * @brief The unique identifier of the operation type.
     */
    OperationType type;
    
    /**
     * @brief A human-readable name for the operation instance.
     * Useful for UI history stack (e.g., "Brightness (+0.5)").
     */
    std::string name;
 
    /**
     * @brief Whether this operation is currently active.
     * If false, the pipeline should skip this operation.
     */
    bool enabled {true};
  
    /**
     * @brief Generic parameter storage.
     * * Stores configuration values mapped by string keys.
     * - Key: Parameter name (e.g., "value", "radius", "mode").
     * - Value: std::any containing the data (float, int, string, vector, etc.).
     * * @note Operations are responsible for validating the presence and type
     * of their required parameters.
     */
    std::unordered_map<std::string, std::any> params;
    
    /**
     * @brief Helper to get a parameter value safely.
     * @tparam T The expected type of the parameter.
     * @param key The parameter name.
     * @param defaultValue Value to return if key is missing or type mismatch.
     * @return The parameter value or defaultValue.
     */
    template <typename T>
    T getParam(const std::string& key, const T& defaultValue) const {
        auto it = params.find(key);
        if (it != params.end()) {
            try {
                return std::any_cast<T>(it->second);
            } catch (const std::bad_any_cast&) {
                // Type mismatch, fallback to default
            }
        }
        return defaultValue;
    }

    /**
     * @brief Helper to set a parameter value easily.
     * @tparam T The type of the value.
     * @param key The parameter name.
     * @param value The value to store.
     */
    template <typename T>
    void setParam(const std::string& key, const T& value) {
        params[key] = value;
    }
};


} // namespace Operations

} // namespace CaptureMoment::Core
