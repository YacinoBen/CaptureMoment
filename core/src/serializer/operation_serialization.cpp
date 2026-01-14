/**
 * @file operation_serialization.cpp
 * @brief Implementation of OperationSerialization
 * @author CaptureMoment Team
 * @date 2025
 */

#include "serializer/operation_serialization.h"
#include "utils/to_string_utils.h"

#include <spdlog/spdlog.h>
#include <typeinfo>
#include <charconv> // For std::from_chars
#include <string>
#include <cstdlib>   // For std::strtof, std::strtod (fallback potential)

namespace CaptureMoment::Core::Serializer::OperationSerialization {

// Helper functions for internal use
namespace {

// Type tags for encoding
constexpr char TYPE_FLOAT = 'f';
constexpr char TYPE_DOUBLE = 'd';
constexpr char TYPE_INT = 'i';
constexpr char TYPE_BOOL = 'b';
constexpr char TYPE_STRING = 's';

// Template for deserializing floating-point types
template<typename T>
[[nodiscard]] std::any deserializeFloating(std::string_view data_str)
{
    if (data_str.empty()) {
        spdlog::warn("OperationSerialization::deserializeFloating: Empty string view provided for type {}.", typeid(T).name());
        return {};
    }

    T val {};
#if defined(__cpp_lib_to_chars) && __cpp_lib_to_chars >= 201611L
    // Try to use std::from_chars if the feature test macro is defined
    auto [ptr, ec] { std::from_chars(data_str.data(), data_str.data() + data_str.size(), val) };
    if (ec == std::errc() && ptr == data_str.data() + data_str.size()) { // Check full consumption
        return val;
    } else {
        spdlog::warn("OperationSerialization::deserializeFloating: std::from_chars failed to parse {} from: '{}', error_code: {}", typeid(T).name(), data_str, static_cast<int>(ec));
        // Return empty if from_chars fails
        return {};
    }
#else
    // If the macro is not defined, use std::stof or std::stod
    std::string str_to_parse(data_str);
    try {
        if constexpr (std::is_same_v<T, float>) {
            val = std::stof(str_to_parse);
        } else if constexpr (std::is_same_v<T, double>) {
            val = std::stod(str_to_parse);
        }
        // Add other floating-point types if needed (long double)
        // else if constexpr (std::is_same_v<T, long double>) { ... }
        return val;
    } catch (const std::invalid_argument& e) {
        spdlog::warn("OperationSerialization::deserializeFloating: std::stof/stod - Invalid argument for {} parsing from: '{}', error: {}", typeid(T).name(), data_str, e.what());
        return {};
    } catch (const std::out_of_range& e) {
        spdlog::warn("OperationSerialization::deserializeFloating: std::stof/stod - Out of range for {} parsing from: '{}', error: {}", typeid(T).name(), data_str, e.what());
        return {};
    } catch (...) {
        spdlog::warn("OperationSerialization::deserializeFloating: std::stof/stod - Unknown error during {} parsing from: '{}'", typeid(T).name(), data_str);
        return {};
    }
#endif
}

// Deserialization helpers
[[nodiscard]] std::any deserializeInt(std::string_view data_str)
{
    if (data_str.empty()) {
        spdlog::warn("OperationSerialization::deserializeInt: Empty string view provided.");
        return {};
    }

    int val {0};
    // std::from_chars for integers is reliable on C++17+ standard libraries
    auto [ptr, ec] { std::from_chars(data_str.data(), data_str.data() + data_str.size(), val) };
    if (ec == std::errc() && ptr == data_str.data() + data_str.size()) { // Check full consumption
        return val;
    } else {
        spdlog::warn("OperationSerialization::deserializeInt: Failed to parse int from: '{}', error_code: {}", data_str, static_cast<int>(ec));
        return {};
    }
}

[[nodiscard]] std::any deserializeBool(std::string_view data_str)
{
    if (data_str == "true" || data_str == "True" || data_str == "TRUE" || data_str == "1") {
        return true;
    } else if (data_str == "false" || data_str == "False" || data_str == "FALSE" || data_str == "0") {
        return false;
    } else {
        spdlog::warn("OperationSerialization::deserializeBool: Failed to parse bool from: '{}'", data_str);
        return {};
    }
}

[[nodiscard]] std::any deserializeString(std::string_view data_str) {
    // If escaping was applied during serialization, it would be undone here.
    return std::string(data_str); // Direct conversion
}

} // anonymous namespace

std::string serializeParameter(const std::any& value)
{
    const std::type_info& type { value.type() };

    if (type == typeid(float))
    {
        return TYPE_FLOAT + std::string(":") + CaptureMoment::Core::utils::toString(std::any_cast<float>(value));
    } else if (type == typeid(double)) {
        return TYPE_DOUBLE + std::string(":") + CaptureMoment::Core::utils::toString(std::any_cast<double>(value));
    } else if (type == typeid(int)) {
        return TYPE_INT + std::string(":") + CaptureMoment::Core::utils::toString(std::any_cast<int>(value));
    } else if (type == typeid(bool)) {
        return TYPE_BOOL + std::string(":") + CaptureMoment::Core::utils::toString(std::any_cast<bool>(value));
    } else if (type == typeid(std::string)) {
        return TYPE_STRING + std::string(":") + CaptureMoment::Core::utils::toString(std::any_cast<std::string>(value));
    }
    // Add more types as needed...

    spdlog::warn("OperationSerialization::serializeParameter: Unsupported type for serialization: {}", type.name());
    return {};
}

std::any deserializeParameter(std::string_view value_str)
{
    if (value_str.empty()) {
        spdlog::warn("OperationSerialization::deserializeParameter: Empty value string.");
        return {};
    }

    size_t colon_pos { value_str.find(':') };
    if (colon_pos == std::string_view::npos) {
        spdlog::warn("OperationSerialization::deserializeParameter: Invalid format, missing colon in: '{}'", value_str);
        return {};
    }

    char type_tag { value_str[0] };
    std::string_view data_str { value_str.substr(colon_pos + 1) };

    if (data_str.empty()) {
        spdlog::warn("OperationSerialization::deserializeParameter: Invalid format, empty data in: '{}'", value_str);
        return {};
    }

    switch (type_tag)
    {
    case TYPE_FLOAT:
        return deserializeFloating<float>(data_str);
    case TYPE_DOUBLE:
        return deserializeFloating<double>(data_str);
    case TYPE_INT:
        return deserializeInt(data_str);
    case TYPE_BOOL:
        return deserializeBool(data_str);
    case TYPE_STRING:
        return deserializeString(data_str);
    // Add more cases as needed...
    default:
        spdlog::warn("OperationSerialization::deserializeParameter: Unsupported type tag '{}' for value: '{}'", type_tag, value_str);
        return {};
    }
}

std::string serializeOperationParameters(const Operations::OperationDescriptor& descriptor)
{
    // This is a simple implementation concatenating serialized parameters.
    // A more robust implementation might use a structured format (JSON, XML-like) or a map representation.
    std::ostringstream oss;
    for (const auto& [param_name, param_value] : descriptor.params)
    {
        std::string serialized_val { serializeParameter(param_value) };
        if (!serialized_val.empty()) { // Only add if serialization was successful
            oss << param_name << "=" << serialized_val << ";"; // Use a delimiter like '=' and ';'
        } else {
            spdlog::warn("OperationSerialization::serializeOperationParameters: Could not serialize parameter '{}' for operation '{}'. Skipping.", param_name, descriptor.name);
        }
    }
    return oss.str();
}

bool deserializeOperationParameters(std::string_view params_str, Operations::OperationDescriptor& descriptor)
{
    // Clear existing parameters
    descriptor.params.clear();

    if (params_str.empty()) {
        return true; // Nothing to deserialize, considered successful
    }

    // This is a simple parser for the format "name1=value1;name2=value2;"
    // Use string_view operations to avoid std::istringstream and getline issues with MSVC
    std::string_view remaining { params_str };
    size_t start {0};
    size_t end {0};

    while (start < remaining.size())
    {
        // Find the end of the current entry (delimited by ';')
        end = remaining.find(';', start);
        if (end == std::string_view::npos) {
            // No more ';' found, use the rest of the string
            end = remaining.size();
        }

        std::string_view param_entry { remaining.substr(start, end - start) };

        if (!param_entry.empty()) { // Check if entry is not empty
            // Find the '=' delimiter within the entry
            size_t equals_pos { param_entry.find('=') };
            if (equals_pos != std::string_view::npos) {
                std::string_view param_name_view { param_entry.substr(0, equals_pos) };
                std::string_view param_value_str_view { param_entry.substr(equals_pos + 1) };

                // Convert string_views to strings for the map key and the deserialization function
                std::string param_name{param_name_view};
                std::string param_value_str{param_value_str_view};

                std::any deserialized_value = deserializeParameter(param_value_str);
                if (deserialized_value.has_value()) {
                    descriptor.params[param_name] = deserialized_value;
                    spdlog::debug("OperationSerialization::deserializeOperationParameters: Parsed parameter '{}' with value (type {}) for operation: '{}'", param_name, deserialized_value.type().name(), descriptor.name);
                } else {
                    spdlog::warn("OperationSerialization::deserializeOperationParameters: Could not parse parameter '{}' with value '{}' for operation '{}'. Skipping.", param_name, param_value_str, descriptor.name);
                }
            } else {
                spdlog::warn("OperationSerialization::deserializeOperationParameters: Invalid parameter entry format: '{}'", std::string(param_entry)); // Convert for logging
            }
        }

        // Move start to the next character after the ';'
        start = end + 1;
    }

    return true; // Parsing completed (success or with warnings)
}

} // namespace CaptureMoment::Core::Serializer::OperationSerialization
