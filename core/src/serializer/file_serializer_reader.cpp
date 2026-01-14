/**
 * @file file_serializer_reader.cpp
 * @brief Implementation of FileSerializerReader
 * @author CaptureMoment Team
 * @date 2025
 */

#include "serializer/file_serializer_reader.h"
#include "serializer/provider/exiv2_initializer.h"
#include "serializer/operation_serialization.h"
#include <spdlog/spdlog.h>
#include <exiv2/exiv2.hpp>
#include <stdexcept>
#include <any>
#include <magic_enum/magic_enum.hpp>
#include <unordered_map>

namespace CaptureMoment::Core::Serializer {

FileSerializerReader::FileSerializerReader(std::unique_ptr<IXmpProvider> xmp_provider, std::unique_ptr<IXmpPathStrategy> xmp_path_strategy)
    : m_xmp_provider(std::move(xmp_provider)), m_xmp_path_strategy(std::move(xmp_path_strategy))
{
    if (!m_xmp_provider || !m_xmp_path_strategy) {
        spdlog::error("FileSerializerReader: Constructor received a null IXmpProvider or IXmpPathStrategy.");
        throw std::invalid_argument("FileSerializerReader: IXmpProvider and IXmpPathStrategy cannot be null.");
    }
    spdlog::debug("FileSerializerReader: Constructed with IXmpProvider and IXmpPathStrategy.");
}

std::vector<Operations::OperationDescriptor> FileSerializerReader::loadFromFile(std::string_view source_image_path) const
{
    if (source_image_path.empty()) {
         spdlog::error("FileSerializerReader::loadFromFile: Source image path is empty.");
         return {}; // Return an empty vector
    }
    spdlog::debug("FileSerializerReader::loadFromFile: Attempting to load operations for image: {}", source_image_path);

    // Ensure Exiv2 is initialized before any operations
    Exiv2Initializer::initialize();

    // Step 0: Determine the XMP file path using the injected strategy
    std::string xmp_file_path { m_xmp_path_strategy->getXmpPathForImage(source_image_path) };
    spdlog::debug("FileSerializerReader::loadFromFile: Determined XMP file path: {}", xmp_file_path);

    // Step 1: Use the XMP provider to read the packet from the determined XMP file
    std::string xmp_packet { m_xmp_provider->readXmp(xmp_file_path) };
    if (xmp_packet.empty()) {
        spdlog::warn("FileSerializerReader::loadFromFile: IXmpProvider returned an empty XMP packet for file: {} (associated with image: {}). Assuming no operations to load.", xmp_file_path, source_image_path);
        return {}; // Return an empty vector if no packet is found or in case of error
    }

    // Step 2: Parse the XMP packet string into OperationDescriptors
    std::string source_path_from_xmp; // To check potential correspondence
    std::vector<Operations::OperationDescriptor> operations { parseXmpPacket(xmp_packet, source_path_from_xmp) };

    if (operations.empty()) {
        spdlog::warn("FileSerializerReader::loadFromFile: Parsing XMP packet from file: {} (associated with image: {}) resulted in an empty list of operations.", xmp_file_path, source_image_path);
        // We can decide to return the empty vector or log more info about the parsing failure
    } else {
        spdlog::info("FileSerializerReader::loadFromFile: Successfully loaded {} operations from XMP file: {} for image: {}", operations.size(), xmp_file_path, source_image_path);
    }

    return operations;
}

std::vector<Operations::OperationDescriptor> FileSerializerReader::parseXmpPacket(const std::string& xmp_packet, std::string& source_image_path_from_xmp) const
{
    spdlog::debug("FileSerializerReader::parseXmpPacket: Parsing XMP packet (size {}).", xmp_packet.size());

    Exiv2::XmpData xmp_data;

    try {
        // Decode the XMP packet string into the XmpData container
        if (Exiv2::XmpParser::decode(xmp_data, xmp_packet.c_str()) != 0) {
            spdlog::error("FileSerializerReader::parseXmpPacket: Failed to decode XMP packet.");
            return {}; // Return an empty vector in case of decoding failure
        }

        // Check if it's our format
        std::string serialized_by { xmp_data["Xmp.cm.serializedBy"].toString() };
        if (serialized_by != "CaptureMoment") {
             spdlog::warn("FileSerializerReader::parseXmpPacket: XMP packet is not marked as serialized by CaptureMoment (found '{}'). Skipping.", serialized_by);
             return {};
        }

        // Optionally, read the source image path stored in the XMP
        source_image_path_from_xmp = xmp_data["Xmp.cm.sourceImagePath"].toString(); // Returns an empty string if the key does not exist
        spdlog::debug("FileSerializerReader::parseXmpPacket: Found source image path in XMP: '{}'", source_image_path_from_xmp);

        std::vector<Operations::OperationDescriptor> operations;
        // Iterate through potential operations in the XMP data
        // This assumes operations are stored in an array-like structure Xmp.cm.operation[1], Xmp.cm.operation[2], etc.
        size_t index {1};
        while (true) {
            std::string index_str { std::to_string(index) };
            std::string type_key_str { "Xmp.cm.operation[" + index_str + "].type"};
            std::string name_key_str { "Xmp.cm.operation[" + index_str + "].name"};
            std::string enabled_key_str { "Xmp.cm.operation[" + index_str + "].enabled"};

            // Create Exiv2::XmpKey objects
            Exiv2::XmpKey type_key(type_key_str);
            Exiv2::XmpKey name_key(name_key_str);
            Exiv2::XmpKey enabled_key(enabled_key_str);

            // Use the XmpKey objects with findKey
            auto type_kv { xmp_data.findKey(type_key) };
            auto name_kv { xmp_data.findKey(name_key) };
            auto enabled_kv { xmp_data.findKey(enabled_key) };

            if (type_kv == xmp_data.end()) {
                // No more operations found with this index
                break;
            }

            Operations::OperationDescriptor op_desc;

            // Read Type (using magic_enum)
            std::string type_str { type_kv->toString() };
            auto type_opt = magic_enum::enum_cast<Operations::OperationType>(type_str);
            if (!type_opt.has_value()) {
                spdlog::warn("FileSerializerReader::parseXmpPacket: Unknown OperationType '{}' found for operation index {}. Skipping operation.", type_str, index);
                index++;
                continue; // Move to the next operation
            }
            op_desc.type = type_opt.value();

            // Read Name
            if (name_kv != xmp_data.end()) {
                op_desc.name = name_kv->toString();
            } else {
                // Generate a default name if not found ?
                op_desc.name = type_str + " (from XMP)";
            }

            // Read Enabled flag
            if (enabled_kv != xmp_data.end())
            {
                std::string enabled_str { enabled_kv->toString() };
                // Handle various boolean representations
                if (enabled_str == "true" || enabled_str == "True" || enabled_str == "TRUE" || enabled_str == "1") {
                    op_desc.enabled = true;
                } else if (enabled_str == "false" || enabled_str == "False" || enabled_str == "FALSE" || enabled_str == "0") {
                    op_desc.enabled = false;
                } else {
                    spdlog::warn("FileSerializerReader::parseXmpPacket: Invalid boolean value '{}' for enabled flag of operation index {}. Using default (true).", enabled_str, index);
                    op_desc.enabled = true;
                }
            } else {
                op_desc.enabled = true; // Default value
            }

            // Read Parameters
            // Iterate through keys that start with "Xmp.cm.operation[<index>].param."
            std::string param_prefix { "Xmp.cm.operation[" + index_str + "].param." };
            for (const auto& kv : xmp_data) {
                std::string key = kv.key();
                if (key.starts_with(param_prefix))
                {
                    std::string param_name { key.substr(param_prefix.length()) };
                    std::string param_value_str { kv.toString() };

                    // Deserialize the value using the robust typed approach via OperationSerialization
                    std::any parsed_value { OperationSerialization::deserializeParameter(param_value_str) };
                    if (parsed_value.has_value()) {
                         op_desc.params[param_name] = parsed_value;
                         spdlog::debug("FileSerializerReader::parseXmpPacket: Parsed parameter '{}' with value (type {}) for operation {}: '{}'", param_name, parsed_value.type().name(), op_desc.name, param_value_str);
                    } else {
                         spdlog::warn("FileSerializerReader::parseXmpPacket: Could not parse parameter '{}' with value '{}' for operation {}. Skipping.", param_name, param_value_str, op_desc.name);
                    }
                }
            }

            operations.push_back(op_desc);
            spdlog::debug("FileSerializerReader::parseXmpPacket: Parsed operation index {}: type={}, name={}, enabled={}", index, magic_enum::enum_name(op_desc.type), op_desc.name, op_desc.enabled);
            index++;
        }

        spdlog::debug("FileSerializerReader::parseXmpPacket: Successfully parsed {} operations from XMP packet.", operations.size());
        return operations;

    } catch (const Exiv2::Error& e) {
        spdlog::error("FileSerializerReader::parseXmpPacket: Exiv2 error during parsing: {}", e.what());
        return {}; // Return an empty vector in case of Exiv2 error
    } catch (const std::exception& e) {
        spdlog::error("FileSerializerReader::parseXmpPacket: General error during parsing: {}", e.what());
        return {}; // Return an empty vector in case of general error
    }
    // No explicit return outside catches needed, as all paths return above.
}

} // namespace CaptureMoment::Core::Serializer
