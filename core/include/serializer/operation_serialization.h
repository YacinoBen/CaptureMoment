/**
 * @file operation_serialization.h
 * @brief Declaration of OperationSerialization interface
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <string>
#include <string_view>
#include "operations/operation_descriptor.h"

namespace CaptureMoment::Core {

namespace Serializer {
/**
 * @brief Namespace containing utilities for serializing and deserializing OperationDescriptor parameters.
 * This isolates the logic for converting OperationValue (std::variant) parameter values to/from string representations.
 */

/**
 * @brief Serializes a single OperationValue (variant) to a string representation.
 * This function converts the value directly to a string without type tags.
 * For example: Float 3.14 -> "3.14", Bool true -> "true", String "hello" -> "hello".
 *
 * @param value The OperationValue (variant) to serialize.
 * @return A string representing the value. Returns an empty string if the type is unsupported.
 */
[[nodiscard]] std::string serializeParameter(const Operations::OperationValue& value);

/**
 * @brief Deserializes a string representation back into an OperationValue.
 * This function attempts to infer the type from the string content (e.g., "true" -> Bool, "3.14" -> Float, "10" -> Int).
 *
 * @param value_str The string representation of the value.
 * @return An OperationValue (variant) containing the deserialized value.
 *         Defaults to std::string if the value doesn't match specific numeric or boolean patterns.
 */
[[nodiscard]] Operations::OperationValue deserializeParameter(std::string_view value_str);

/**
 * @brief Serializes an entire OperationDescriptor's parameters map to a string representation.
 * Iterates through the 'params' map and serializes each value using serializeParameter.
 *
 * @param descriptor The OperationDescriptor containing the parameters to serialize.
 * @return A string representing the serialized parameters map.
 */
[[nodiscard]] std::string serializeOperationParameters(const Operations::OperationDescriptor& descriptor);

/**
 * @brief Deserializes a string representation back into an OperationDescriptor's parameters map.
 * Parses the input string and reconstructs the 'params' map using deserializeParameter.
 *
 * @param params_str The string representation of the parameters map.
 * @param descriptor The OperationDescriptor to populate with the deserialized parameters.
 *                   Its 'params' map will be cleared and filled.
 * @return true if the deserialization was successful, false otherwise.
 */
[[nodiscard]] bool deserializeOperationParameters(std::string_view params_str, Operations::OperationDescriptor& descriptor);

} // namespace Serializer

} // namespace CaptureMoment::Core
