/**
 * @file i_file_serializer_reader.h
 * @brief Declaration of IFileSerializerReader interface
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <string_view>
#include <vector>

#include "operations/operation_descriptor.h"

namespace CaptureMoment::Core {

namespace Serializer {

/**
 * @brief Interface for reading serialized operation data from a file.
 */
class IFileSerializerReader {
public:
    virtual ~IFileSerializerReader() = default;

    /**
     * @brief Loads a list of operations from a file in the target format (e.g., XMP).
     * The source file path is determined by the injected IXmpPathStrategy based on the source image path.
     * @param source_image_path The path to the source image file. Used by the path strategy to determine the source XMP file location.
     * @return A vector of OperationDescriptors loaded from the file, or an empty vector if loading failed.
     */
    [[nodiscard]] virtual std::vector<Operations::OperationDescriptor> loadFromFile(std::string_view source_image_path) const = 0;
};

} // namespace Serializer

} // namespace CaptureMoment::Core
