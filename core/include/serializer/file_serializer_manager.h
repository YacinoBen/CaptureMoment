/**
 * @file file_serializer_manager.h
 * @brief Declaration of FileSerializerManager class
 * @details High-level manager for abstracting file-based serialization logic.
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include "serializer/i_file_serializer_writer.h"
#include "serializer/i_file_serializer_reader.h"

#include <memory>
#include <span>
#include <string_view>
#include <vector>

#include "operations/operation_descriptor.h"

namespace CaptureMoment::Core {

namespace Serializer {

/**
 * @brief High-level manager orchestrating file-based serialization and deserialization of operation data.
 *
 * This class abstracts the complexities of choosing and configuring specific serializers (Writer/Reader),
 * XMP providers, and path strategies. It provides a unified interface for saving and loading operations
 * associated with image files.
 *
 * **Thread Safety:** This class is not inherently thread-safe if the same instance is shared across threads.
 * External synchronization is required if used concurrently.
 */
class FileSerializerManager {
public:
    /**
     * @brief Constructs a FileSerializerManager.
     *
     * @param writer A unique pointer to the writer implementation (e.g., FileSerializerWriter).
     *               Must not be null.
     * @param reader A unique pointer to the reader implementation (e.g., FileSerializerReader).
     *               Must not be null.
     */
    explicit FileSerializerManager(
        std::unique_ptr<IFileSerializerWriter> writer,
        std::unique_ptr<IFileSerializerReader> reader
        );

    ~FileSerializerManager() = default;

    // Disable copy/move for simplicity, assuming unique_ptr dependencies
    FileSerializerManager(const FileSerializerManager&) = delete;
    FileSerializerManager& operator=(const FileSerializerManager&) = delete;
    FileSerializerManager(FileSerializerManager&&) = delete;
    FileSerializerManager& operator=(FileSerializerManager&&) = delete;

    /**
     * @brief Saves the provided list of operations to a file associated with the source image path.
     *
     * The specific file format and location are determined by the injected IFileSerializerWriter
     * and its dependencies (IXmpProvider, IXmpPathStrategy).
     *
     * @param source_image_path The path to the source image file. Used to determine the target file location.
     * @param operations The span of OperationDescriptors to serialize.
     * @return true if the save operation was successful, false otherwise.
     */
    [[nodiscard]] bool saveToFile(
        std::string_view source_image_path,
        std::span<const Operations::OperationDescriptor> operations
        ) const;

    /**
     * @brief Loads a list of operations from a file associated with the source image path.
     *
     * The specific file format and location are determined by the injected IFileSerializerReader
     * and its dependencies (IXmpProvider, IXmpPathStrategy).
     *
     * @param source_image_path The path to the source image file. Used to determine the source file location.
     * @return A vector of OperationDescriptors loaded from the file, or an empty vector if loading failed.
     */
    [[nodiscard]] std::vector<Operations::OperationDescriptor> loadFromFile(std::string_view source_image_path) const;

private:
    /**
     * @brief The serializer responsible for writing operation data.
     */
    std::unique_ptr<IFileSerializerWriter> m_writer;

    /**
     * @brief The deserializer responsible for reading operation data.
     */
    std::unique_ptr<IFileSerializerReader> m_reader;
};

} // Serializer
} // namespace CaptureMoment::Core
