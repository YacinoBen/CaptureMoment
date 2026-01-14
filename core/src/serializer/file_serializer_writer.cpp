/**
 * @file file_serializer_writer.cpp
 * @brief Implementation of FileSerializerWriter
 * @author CaptureMoment Team
 * @date 2025
 */

#include "serializer/file_serializer_writer.h"
#include "serializer/provider/exiv2_initializer.h"
#include "serializer/operation_serialization.h"
#include <spdlog/spdlog.h>
#include <exiv2/exiv2.hpp>
#include <stdexcept>
#include <ranges>
#include <magic_enum/magic_enum.hpp>

namespace CaptureMoment::Core::Serializer {

FileSerializerWriter::FileSerializerWriter(std::unique_ptr<IXmpProvider> xmp_provider, std::unique_ptr<IXmpPathStrategy> xmp_path_strategy)
    : m_xmp_provider(std::move(xmp_provider)), m_xmp_path_strategy(std::move(xmp_path_strategy))
{
    if (!m_xmp_provider || !m_xmp_path_strategy) {
        spdlog::error("FileSerializerWriter: Constructor received a null IXmpProvider or IXmpPathStrategy.");
        throw std::invalid_argument("FileSerializerWriter: IXmpProvider and IXmpPathStrategy cannot be null.");
    }
    spdlog::debug("FileSerializerWriter: Constructed with IXmpProvider and IXmpPathStrategy.");
}

bool FileSerializerWriter::saveToFile(std::string_view source_image_path, std::span<const Operations::OperationDescriptor> operations) const
{
    if (source_image_path.empty()) {
         spdlog::error("FileSerializerWriter::saveToFile: Source image path is empty.");
         return false;
    }
    spdlog::debug("FileSerializerWriter::saveToFile: Attempting to save {} operations for image: {}", operations.size(), source_image_path);

    // Ensure Exiv2 is initialized before any operations
    Exiv2Initializer::initialize();

    // Step 0: Determine the XMP file path using the injected strategy
    std::string xmp_file_path { m_xmp_path_strategy->getXmpPathForImage(source_image_path) };
    spdlog::debug("FileSerializerWriter::saveToFile: Determined XMP file path: {}", xmp_file_path);

    // Step 1: Convert operations to XMP string, including the source image path metadata
    std::string xmp_packet { serializeOperationsToXmp(operations, source_image_path) };
    if (xmp_packet.empty()) {
        spdlog::error("FileSerializerWriter::saveToFile: Failed to serialize operations to XMP for image: {}", source_image_path);
        return false;
    }

    // Step 2: Use the XMP provider to write the packet to the determined XMP file
    bool write_success { m_xmp_provider->writeXmp(xmp_file_path, xmp_packet) };

    if (write_success) {
        spdlog::info("FileSerializerWriter::saveToFile: Successfully saved operations to XMP file: {} for image: {}", xmp_file_path, source_image_path);
    } else {
        spdlog::error("FileSerializerWriter::saveToFile: IXmpProvider failed to write XMP data to file: {} for image: {}", xmp_file_path, source_image_path);
    }

    return write_success;
}

std::string FileSerializerWriter::serializeOperationsToXmp(std::span<const Operations::OperationDescriptor> operations, std::string_view source_image_path) const
{
   spdlog::debug("FileSerializerWriter::serializeOperationsToXmp: Serializing {} operations for image: {}", operations.size(), source_image_path);

    // Example XMP structure (you would define your own schema)
    // Register a custom namespace for your application's operations
    const std::string ns_uri { "https://github.com/YacinoBen/CaptureMoment/" };
    const std::string ns_prefix { "cm" };

    // Exiv2 requires registering custom namespaces before using them
    // Note: This registration is usually global or done once per process, not per serialization call.
    // For simplicity here, we assume it's handled or done elsewhere if needed frequently.
    // Exiv2::XmpProperties::registerNs(ns_uri, ns_prefix); // Could cause issues if called repeatedly or concurrently

    Exiv2::XmpData xmp_data;

    try {
        // Add metadata about the serialization itself
        xmp_data["Xmp.cm.serializedBy"] = "CaptureMoment";
        xmp_data["Xmp.cm.version"] = "1.0"; // App version
        // Include the source image path as a metadata field within the XMP
        xmp_data["Xmp.cm.sourceImagePath"] = source_image_path.data();


        // Iterate through operations with index using a classic loop (C++20 compatible)
        for (size_t i = 0; i < operations.size(); ++i) {
            const auto& op = operations[i]; // 'op' is a const reference to the current OperationDescriptor
            std::string index_str { std::to_string(i + 1) }; // Start indexing from 1 for readability in XMP

            // Use magic_enum directly here
            std::string_view type_name_view { magic_enum::enum_name(op.type) };
            xmp_data["Xmp.cm.operation[" + index_str + "].type"] = type_name_view.data(); // .data() to get const char*
            xmp_data["Xmp.cm.operation[" + index_str + "].name"] = op.name.c_str();
            xmp_data["Xmp.cm.operation[" + index_str + "].enabled"] = op.enabled;

            // Iterate through the generic parameters map using range-for
            for (const auto& [param_name, param_value] : op.params)
            {
                // Use the dedicated OperationSerialization service
                std::string serialized_param_value { OperationSerialization::serializeParameter(param_value) };
                if (!serialized_param_value.empty()) { // Only add if serialization was successful
                    std::string xmp_param_key { "Xmp.cm.operation[" + index_str + "].param." + param_name };
                    xmp_data[xmp_param_key] = serialized_param_value.c_str(); // .c_str() for Exiv2
                } else {
                    spdlog::warn("FileSerializerWriter::serializeOperationsToXmp: Could not serialize parameter '{}' for operation '{}' (index {}). Skipping.", param_name, op.name, i);
                }
            }
        }

        // Serialize the XMP data container to a packet string
        std::string xmp_packet;
        if (Exiv2::XmpParser::encode(xmp_packet, xmp_data) != 0) {
            spdlog::error("FileSerializerWriter::serializeOperationsToXmp: Failed to encode XMP packet.");
            return {}; // Return empty string on failure
        }

        spdlog::debug("FileSerializerWriter::serializeOperationsToXmp: Successfully serialized to XMP packet (size {}).", xmp_packet.size());
        return xmp_packet;

    } catch (const Exiv2::Error& e) {
        spdlog::error("FileSerializerWriter::serializeOperationsToXmp: Exiv2 error during serialization: {}", e.what());
        return {}; // Return empty string on failure
    } catch (const std::exception& e) {
        spdlog::error("FileSerializerWriter::serializeOperationsToXmp: General error during serialization: {}", e.what());
        return {}; // Return empty string on failure
    }
    // No explicit return outside catches needed, as all paths return above.
}

} // namespace CaptureMoment::Core::Serializer
