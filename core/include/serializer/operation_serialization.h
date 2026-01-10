/**
 * @file operation_serialization.h
 * @brief Declaration of OperationSerialization interface
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <any>
#include <string_view>
#include "operations/operation_descriptor.h" 

namespace CaptureMoment::Core {

namespace Serializer {
/**
 * @brief Namespace containing utilities for serializing and deserializing OperationDescriptor parameters.
 * This isolates the logic for converting std::any parameter values to/from string representations
 * with type information, making it reusable across different serialization formats (XMP, JSON, etc.).
 */
namespace OperationSerialization {

    /**
     * @brief Serializes a single std::any parameter value to a string representation.
     * This function embeds the type information into the string (e.g., "f:0.5", "i:42", "b:true", "s:text").
     * @param value The std::any value to serialize.
     * @return A string representing the value with its type tag, or an empty string if the type is not supported.
     */
    [[nodiscard]] std::string serializeParameter(const std::any& value);

    /**
     * @brief Deserializes a string representation back into an std::any value.
     * This function expects the string to be in the format "type_tag:value" (e.g., "f:0.5").
     * @param value_str The string representation of the value, including the type tag.
     * @return An std::any containing the deserialized value, or an empty std::any if the type tag is unknown
     *         or the deserialization fails.
     */
    [[nodiscard]] std::any deserializeParameter(std::string_view value_str);

    /**
     * @brief Serializes an entire OperationDescriptor's parameters map to a string representation.
     * This function iterates through the 'params' map of the descriptor and serializes each value
     * individually using serializeParameter.
     * @param descriptor The OperationDescriptor containing the parameters to serialize.
     * @return A string representing the serialized parameters map.
     *         The format is implementation-specific (e.g., a simple concatenated string or a structured format).
     */
    [[nodiscard]] std::string serializeOperationParameters(const Operations::OperationDescriptor& descriptor);

    /**
     * @brief Deserializes a string representation back into an OperationDescriptor's parameters map.
     * This function parses the input string and reconstructs the 'params' map, using deserializeParameter
     * for each individual value.
     * @param params_str The string representation of the parameters map.
     * @param descriptor The OperationDescriptor to populate with the deserialized parameters.
     *                   Its 'params' map will be cleared and filled.
     * @return true if the deserialization was successful, false otherwise.
     */
    [[nodiscard]] bool deserializeOperationParameters(std::string_view params_str, Operations::OperationDescriptor& descriptor);

} // namespace OperationSerialization

} // namespace Serializer

} // namespace CaptureMoment::Core
