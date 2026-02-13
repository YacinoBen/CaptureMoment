/**
 * @file file_serializer_manager.cpp
 * @brief Implementation of FileSerializerManager
 * @details Implements the delegation to the specific Writer and Reader implementations.
 * @author CaptureMoment Team
 * @date 2025
 */

#include "serializer/file_serializer_manager.h"
#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::Serializer {

FileSerializerManager::FileSerializerManager(
    std::unique_ptr<IFileSerializerWriter> writer,
    std::unique_ptr<IFileSerializerReader> reader)
    : m_writer(std::move(writer))
    , m_reader(std::move(reader))
{
    if (!m_writer || !m_reader) {
        spdlog::error("FileSerializerManager: Constructor received a null IFileSerializerWriter or IFileSerializerReader.");
        throw std::invalid_argument("FileSerializerManager: Writer and Reader cannot be null.");
    }
    spdlog::debug("FileSerializerManager: Constructed with IFileSerializerWriter and IFileSerializerReader.");
}

bool FileSerializerManager::saveToFile(std::string_view source_image_path, std::span<const Operations::OperationDescriptor> operations) const
{
    if (source_image_path.empty()) {
        spdlog::error("FileSerializerManager::saveToFile: Source image path is empty.");
        return false;
    }

    if (operations.empty()) {
        spdlog::info("FileSerializerManager::saveToFile: Operations list is empty. Saving empty list to XMP.");
        // Saving an empty list is allowed (clearing XMP).
    }

    spdlog::debug("FileSerializerManager::saveToFile: Attempting to save {} operations for image: {}", operations.size(), source_image_path);

    // Delegate the actual saving to the injected writer
    // The Writer handles the conversion to XMP and the file I/O.
    bool success = m_writer->saveToFile(source_image_path, operations);

    if (success) {
        spdlog::info("FileSerializerManager::saveToFile: Successfully saved {} operations to file for image: {}", operations.size(), source_image_path);
    } else {
        spdlog::error("FileSerializerManager::saveToFile: Failed to save operations to file for image: {}", source_image_path);
    }

    return success;
}

std::vector<Operations::OperationDescriptor> FileSerializerManager::loadFromFile(std::string_view source_image_path) const
{
    if (source_image_path.empty()) {
        spdlog::error("FileSerializerManager::loadFromFile: Source image path is empty.");
        return {}; // Return an empty vector
    }

    spdlog::debug("FileSerializerManager::loadFromFile: Attempting to load operations for image: {}", source_image_path);

    // Delegate the actual loading to the injected reader
    // The Reader handles the file I/O, XMP parsing, and conversion to OperationDescriptors.
    std::vector<Operations::OperationDescriptor> operations = m_reader->loadFromFile(source_image_path);

    if (operations.empty()) {
        spdlog::info("FileSerializerManager::loadFromFile: No operations loaded from file for image: {} (file might not exist or be empty)", source_image_path);
    } else {
        spdlog::info("FileSerializerManager::loadFromFile: Successfully loaded {} operations from file for image: {}", operations.size(), source_image_path);
    }

    return operations;
}

} // namespace CaptureMoment::Core::Serializer
