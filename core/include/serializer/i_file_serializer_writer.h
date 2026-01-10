/**
 * @file i_file_serializer_writer.h
 * @brief Declaration of IFileSerializerWriter interface
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <string_view>
#include <span>
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
     * @param filePath The path to the file where the data should be saved.
     * @param operations The span of OperationDescriptors to serialize.
     * @return true if the save operation was successful, false otherwise.
     */
    [[nodiscard]] virtual bool saveToFile(std::string_view filePath, std::span<const Common::OperationDescriptor> operations) const = 0;
};

} // namespace Serializer

} // namespace CaptureMoment::Core
