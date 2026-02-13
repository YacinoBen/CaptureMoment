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
#include <magic_enum/magic_enum.hpp>

namespace CaptureMoment::Core::Serializer {

FileSerializerWriter::FileSerializerWriter(
    std::unique_ptr<IXmpProvider> xmp_provider,
    std::unique_ptr<IXmpPathStrategy> xmp_path_strategy)
    : m_xmp_provider(std::move(xmp_provider))
    , m_xmp_path_strategy(std::move(xmp_path_strategy))
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

std::string FileSerializerWriter::serializeOperationsToXmp(
    std::span<const Operations::OperationDescriptor> operations,
    std::string_view source_image_path) const
{
    spdlog::debug("FileSerializerWriter::serializeOperationsToXmp: Serializing {} operations for image: {}", operations.size(), source_image_path);

    // XMP Namespace configuration for "CaptureMoment" (cm)
    const std::string ns_uri { "https://github.com/YacinoBen/CaptureMoment/" };
    const std::string ns_prefix { "cm" };

    // Register the namespace for Exiv2 to recognize "Xmp.cm.*" keys correctly
    // Note: This is generally safe to call multiple times or ensure it's called once globally.
    Exiv2::XmpProperties::registerNs(ns_uri, ns_prefix);

    Exiv2::XmpData xmp_data;

    try {
        // Add metadata about the serialization itself
        xmp_data["Xmp.cm.serializedBy"] = "CaptureMoment";
        xmp_data["Xmp.cm.version"] = "1.0";
        // Include the source image path as a metadata field within the XMP
        xmp_data["Xmp.cm.sourceImagePath"] = std::string(source_image_path);

        // Iterate through operations with index (1-based for readability in XMP)
        for (size_t i = 0; i < operations.size(); ++i) {
            const auto& op = operations[i];
            std::string index_str { std::to_string(i + 1) };

            // Use magic_enum to get the string name of the enum (must match Reader's enum_cast)
            std::string_view type_name_view { magic_enum::enum_name(op.type) };
            xmp_data["Xmp.cm.operation[" + index_str + "].type"] = std::string(type_name_view);
            xmp_data["Xmp.cm.operation[" + index_str + "].name"] = op.name;
            xmp_data["Xmp.cm.operation[" + index_str + "].enabled"] = op.enabled;

            // Iterate through the parameters map
            for (const auto& [param_name, param_value] : op.params)
            {
                // Serialize the value (variant) to string using the service
                // No need to check .has_value() as variant always holds a value
                std::string serialized_param_value { Serializer::serializeParameter(param_value) };

                std::string xmp_param_key { "Xmp.cm.operation[" + index_str + "].param." + param_name };
                xmp_data[xmp_param_key] = serialized_param_value;
            }
        }

        // Serialize the XMP data container to a packet string
        std::string xmp_packet;
        if (Exiv2::XmpParser::encode(xmp_packet, xmp_data) != 0) {
            spdlog::error("FileSerializerWriter::serializeOperationsToXmp: Failed to encode XMP packet.");
            return {};
        }

        spdlog::debug("FileSerializerWriter::serializeOperationsToXmp: Successfully serialized to XMP packet (size {}).", xmp_packet.size());
        return xmp_packet;

    } catch (const Exiv2::Error& e) {
        spdlog::error("FileSerializerWriter::serializeOperationsToXmp: Exiv2 error during serialization: {}", e.what());
        return {};
    } catch (const std::exception& e) {
        spdlog::error("FileSerializerWriter::serializeOperationsToXmp: General error during serialization: {}", e.what());
        return {};
    }
}

} // namespace CaptureMoment::Core::Serializer
