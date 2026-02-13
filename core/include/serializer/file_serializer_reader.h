/**
 * @file file_serializer_reader.h
 * @brief Declaration of FileSerializerReader class
 * @details Concrete implementation of IFileSerializerReader using XMP.
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include "serializer/i_file_serializer_reader.h"
#include "serializer/provider/i_xmp_provider.h"
#include "serializer/strategy/i_xmp_path_strategy.h"

#include <memory>
#include <string>
#include <vector>

namespace CaptureMoment::Core {

namespace Serializer {

/**
 * @brief Concrete implementation of IFileSerializerReader using an IXmpProvider for XMP operations
 * and an IXmpPathStrategy to determine where to read the XMP file from.
 *
 * This class handles reading XMP format from the designated file and converting it to a list of OperationDescriptors.
 */
class FileSerializerReader final : public IFileSerializerReader {
public:
    /**
     * @brief Constructs a FileSerializerReader.
     *
     * @param xmp_provider A unique pointer to the XMP provider implementation (e.g., Exiv2Provider).
     *                     Must not be null.
     * @param xmp_path_strategy A unique pointer to the XMP path strategy implementation.
     *                          Must not be null.
     */
    explicit FileSerializerReader(
        std::unique_ptr<IXmpProvider> xmp_provider,
        std::unique_ptr<IXmpPathStrategy> xmp_path_strategy
        );

    ~FileSerializerReader() override = default;

    // Disable copy/move for simplicity, assuming unique_ptr dependencies
    FileSerializerReader(const FileSerializerReader&) = delete;
    FileSerializerReader& operator=(const FileSerializerReader&) = delete;
    FileSerializerReader(FileSerializerReader&&) = delete;
    FileSerializerReader& operator=(FileSerializerReader&&) = delete;

    /**
     * @brief Loads a list of operations from an XMP file determined by the path strategy.
     *
     * This involves using the IXmpProvider to read the XMP packet from the file path determined by the IXmpPathStrategy,
     * then parsing the XMP packet to reconstruct the list of OperationDescriptors.
     *
     * @param source_image_path The path to the source image file. Used by the path strategy to find the source XMP file.
     * @return A vector of OperationDescriptors loaded from the XMP file, or an empty vector if loading or parsing failed.
     */
    [[nodiscard]] std::vector<Operations::OperationDescriptor> loadFromFile(std::string_view source_image_path) const override;

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
     * @brief Parses an XMP packet string into a vector of OperationDescriptors.
     * Extracts the source image path metadata if present.
     *
     * @param xmp_packet The raw XMP packet string to parse.
     * @param[out] source_image_path_from_xmp The source image path found in the XMP, if any.
     * @return A vector of OperationDescriptors reconstructed from the XMP, or an empty vector on failure.
     */
    [[nodiscard]] std::vector<Operations::OperationDescriptor> parseXmpPacket(const std::string& xmp_packet, std::string& source_image_path_from_xmp) const;
};

} // namespace Serializer

} // namespace CaptureMoment::Core
