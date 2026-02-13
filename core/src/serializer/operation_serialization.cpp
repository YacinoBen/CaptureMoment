/**
 * @file operation_serialization.cpp
 * @brief Implementation of OperationSerialization
 * @details Provides serialization for OperationValue (std::variant) without explicit type tags.
 * @author CaptureMoment Team
 * @date 2025
 */

#include "serializer/operation_serialization.h"
#include "operations/operation_descriptor.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::Serializer {

std::string serializeParameter(const Operations::OperationValue& value)
{
    // Use std::visit to handle each type in the variant
    return std::visit([](auto&& arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, bool>) {
            return arg ? "true" : "false";
        }
        else if constexpr (std::is_same_v<T, int>) {
            return std::to_string(arg);
        }
        else if constexpr (std::is_same_v<T, float>) {
            return std::to_string(arg);
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            return arg;
        }
        else {
            return ""; // Should not happen if all variant types are handled
        }
    }, value);
}

Operations::OperationValue deserializeParameter(std::string_view value_str)
{
    // 1. Check for Boolean (Case-insensitive)
    std::string lower_val(value_str);
    std::transform(lower_val.begin(), lower_val.end(), lower_val.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    if (lower_val == "true" || lower_val == "1") return true;
    if (lower_val == "false" || lower_val == "0") return false;

    // 2. Check for Integer (Int)
    // We verify if the entire string is an integer to avoid confusing "10" with "10.5"
    char* endptr = nullptr;
    long long_val = std::strtol(value_str.data(), &endptr, 10);
    // If we consumed the whole string and it wasn't empty
    if (endptr != value_str.data() && *endptr == '\0' && !value_str.empty()) {
        return static_cast<int>(long_val);
    }

    // 3. Check for Float (Float)
    // std::strtof accepts "10" as 10.0f, so we use it if integer parsing failed
    endptr = nullptr;
    float float_val = std::strtof(value_str.data(), &endptr);
    if (endptr != value_str.data()) {
        return float_val;
    }

    // 4. Default to String (String)
    return std::string(value_str);
}

std::string serializeOperationParameters(const Operations::OperationDescriptor& descriptor)
{
    // Simple format: "param1=value1;param2=value2"
    std::string result;
    for (const auto& [key, val] : descriptor.params) {
        if (!result.empty()) {
            result += ";";
        }
        result += key + "=" + serializeParameter(val);
    }
    return result;
}

bool deserializeOperationParameters(std::string_view params_str, Operations::OperationDescriptor& descriptor)
{
    // Reverse logic: Parse "key1=val1;key2=val2"
    // Note: This is a basic implementation. For a robust format, JSON is preferred.
    descriptor.params.clear();

    size_t start = 0;
    while (start < params_str.size()) {
        size_t end = params_str.find(';', start);
        std::string_view pair = (end == std::string_view::npos)
                                    ? params_str.substr(start)
                                    : params_str.substr(start, end - start);

        size_t eq_pos = pair.find('=');
        if (eq_pos != std::string_view::npos) {
            std::string key(pair.substr(0, eq_pos));
            std::string_view val_view(pair.substr(eq_pos + 1));

            descriptor.params[key] = deserializeParameter(val_view);
        }

        if (end == std::string_view::npos) break;
        start = end + 1;
    }

    return true;
}

} // namespace CaptureMoment::Core::Serialization
