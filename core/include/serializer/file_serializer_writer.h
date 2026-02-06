/**
 * @file file_serializer_writer.h
 * @brief Declaration of FileSerializerWriter class
 * @details Concrete implementation of IFileSerializerWriter using XMP for serialization.
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include "serializer/i_file_serializer_writer.h"
#include "serializer/provider/i_xmp_provider.h"
#include "serializer/strategy/i_xmp_path_strategy.h"

#include <memory>
#include <span>
#include <string>

namespace CaptureMoment::Core {

namespace Serializer {

/**
 * @brief Concrete implementation of IFileSerializerWriter using an IXmpProvider for XMP operations
 * and an IXmpPathStrategy to determine where to store/read the XMP file.
 *
 * This class handles converting OperationDescriptors to XMP format and writing them to the designated XMP file.
 */
class FileSerializerWriter final : public IFileSerializerWriter {
public:
    /**
     * @brief Constructs a FileSerializerWriter.
     *
     * @param xmp_provider A unique pointer to the XMP provider implementation (e.g., Exiv2Provider).
     *                     Must not be null.
     * @param xmp_path_strategy A unique pointer to the XMP path strategy implementation.
     *                          Must not be null.
     */
    explicit FileSerializerWriter(
        std::unique_ptr<IXmpProvider> xmp_provider,
        std::unique_ptr<IXmpPathStrategy> xmp_path_strategy
        );

    ~FileSerializerWriter() override = default;

    // Disable copy/move for simplicity, assuming unique_ptr dependencies
    FileSerializerWriter(const FileSerializerWriter&) = delete;
    FileSerializerWriter& operator=(const FileSerializerWriter&) = delete;
    FileSerializerWriter(FileSerializerWriter&&) = delete;
    FileSerializerWriter& operator=(FileSerializerWriter&&) = delete;

    /**
     * @brief Saves the provided list of operations to an XMP file determined by the path strategy.
     *
     * This involves converting the operations to an XMP packet string, including the source image path metadata,
     * and then using the IXmpProvider to write that packet to the file path determined by the IXmpPathStrategy.
     *
     * @param source_image_path The path to the source image file. Used by the path strategy to find the target XMP file.
     * @param operations The span of OperationDescriptors to serialize.
     * @return true if the save operation was successful, false otherwise.
     */
    [[nodiscard]] bool saveToFile(
        std::string_view source_image_path,
        std::span<const Operations::OperationDescriptor> operations
        ) const override;

private:
    /**
     * @brief Provider responsible for raw XMP file I/O.
     */
    std::unique_ptr<IXmpProvider> m_xmp_provider;

    /**
     * @brief Strategy responsible for determining XMP file paths.
     */
    std::unique_ptr<IXmpPathStrategy> m_xmp_path_strategy;

    /**
     * @brief Converts a span of OperationDescriptors into an XMP packet string representation.
     *
     * Includes the source image path as a metadata field within the XMP.
     *
     * @param operations The span of OperationDescriptors to convert.
     * @param source_image_path The path of the source image, to be included in the XMP metadata.
     * @return A string representing the XMP packet, or an empty string on failure.
     */
    [[nodiscard]] std::string serializeOperationsToXmp(
        std::span<const Operations::OperationDescriptor> operations,
        std::string_view source_image_path
        ) const;
};

} // namespace Serializer

} // namespace CaptureMoment::Core
