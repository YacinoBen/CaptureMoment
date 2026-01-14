
/**
 * @file to_string_utils.h
 * @brief Utility functions for converting primitive types to strings.
 * @author CaptureMoment Team
 * @date 2025
 */

 #pragma once

#include <string>
#include <concepts> // For std::integral, std::floating_point

namespace CaptureMoment::Core {

namespace utils {

/**
 * @brief Concept defining types that can be converted to string using std::to_string.
 * This includes integral types (int, long, etc.) and floating-point types (float, double).
 * Note: std::string and bool are handled separately.
 */
template<typename T>
concept ToStringablePrimitive = (std::integral<T> || std::floating_point<T>) &&
                               (!std::same_as<T, bool>);

/**
 * @brief Converts a primitive value (integral or floating-point) to its string representation.
 *
 * This function uses std::to_string for numeric types.
 * It is designed for primitive types where std::to_string provides the correct formatting.
 *
 * @tparam T The type of the value, constrained by ToStringablePrimitive.
 * @param value The value to convert.
 * @return std::string The string representation of the value.
 */
template<ToStringablePrimitive T>
[[nodiscard]] std::string toString(T value) {
    return std::to_string(value);
}

/**
 * @brief Converts a boolean value to its string representation ("true" or "false").
 *
 * This overload handles the special case of boolean conversion,
 * which differs from the numeric behavior of std::to_string.
 *
 * @param value The boolean value to convert.
 * @return std::string "true" if value is true, "false" otherwise.
 */
[[nodiscard]] std::string toString(bool value) {
    return value ? "true" : "false";
}

/**
 * @brief Converts a string value to its string representation (identity operation).
 *
 * This overload handles the case where the input is already a string,
 * returning a copy of the input string.
 *
 * @param value The string value to convert.
 * @return std::string A copy of the input string.
 */
[[nodiscard]] std::string toString(const std::string& value) {
    return value;
}

} // namespace utils

} // namespace CaptureMoment::Core
