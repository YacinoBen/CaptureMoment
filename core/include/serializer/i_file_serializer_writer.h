/**
 * @file i_file_serializer_writer.h
 * @brief Declaration of IFileSerializerWriter interface
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <span>
#include <string_view>
#include "operations/operation_descriptor.h"

namespace CaptureMoment::Core {

namespace Serializer {

/**
 * @brief Interface for writing serialized operation data to a file.
 */
class IFileSerializerWriter {
public:
    virtual ~IFileSerializerWriter() = default;

    /**
     * @brief Saves the provided list of operations to a file in the target format (e.g., XMP).
     * The target file path is determined by the injected IXmpPathStrategy based on the source image path.
     *
     * @param source_image_path The path to the source image file. Used by the path strategy to determine the target file location.
     * @param operations The span of OperationDescriptors to serialize.
     * @return true if the save operation was successful, false otherwise.
     */
    [[nodiscard]] virtual bool saveToFile(std::string_view source_image_path, std::span<const Operations::OperationDescriptor> operations) const = 0;
};

} // namespace Serializer

} // namespace CaptureMoment::Core
