// core/src/serializer/file_serializer_writer.cpp
#include "serializer/file_serializer_writer.h"
#include "serializer/exiv2_initializer.h"
#include <spdlog/spdlog.h>
#include <exiv2/exiv2.hpp>
#include <sstream>
#include <stdexcept>
#include <any> 
#include <typeinfo>

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

bool FileSerializerWriter::saveToFile(std::string_view source_image_path, std::span<const Common::OperationDescriptor> operations) const
{
    if (source_image_path.empty()) {
         spdlog::error("FileSerializerWriter::saveToFile: Source image path is empty.");
         return false;
    }
    spdlog::debug("FileSerializerWriter::saveToFile: Attempting to save {} operations for image: {}", operations.size(), source_image_path);

    // Ensure Exiv2 is initialized before any operations
    Exiv2Initializer::initialize();

    // Step 0: Determine the XMP file path using the injected strategy
    std::string xmp_file_path = m_xmp_path_strategy->getXmpPathForImage(source_image_path);
    spdlog::debug("FileSerializerWriter::saveToFile: Determined XMP file path: {}", xmp_file_path);

    // Step 1: Convert operations to XMP string, including the source image path metadata
    std::string xmp_packet = serializeOperationsToXmp(operations, source_image_path);
    if (xmp_packet.empty()) {
        spdlog::error("FileSerializerWriter::saveToFile: Failed to serialize operations to XMP for image: {}", source_image_path);
        return false;
    }

    // Step 2: Use the XMP provider to write the packet to the determined XMP file
    bool write_success = m_xmp_provider->writeXmp(xmp_file_path, xmp_packet);

    if (write_success) {
        spdlog::info("FileSerializerWriter::saveToFile: Successfully saved operations to XMP file: {} for image: {}", xmp_file_path, source_image_path);
    } else {
        spdlog::error("FileSerializerWriter::saveToFile: IXmpProvider failed to write XMP data to file: {} for image: {}", xmp_file_path, source_image_path);
    }

    return write_success;
}

std::string FileSerializerWriter::serializeOperationsToXmp(std::span<const Common::OperationDescriptor> operations, std::string_view source_image_path) const
{
   spdlog::debug("FileSerializerWriter::serializeOperationsToXmp: Serializing {} operations for image: {}", operations.size(), source_image_path);

    // Define the XMP structure for CaptureMoment operations
    // Register a custom namespace for your application's operations
    const std::string ns_uri = "https://github.com/YacinoBen/CaptureMoment/";
    const std::string ns_prefix = "cm";

    // Exiv2 requires registering custom namespaces before using them
    // Note: This registration is usually global or done once per process, not per serialization call.
    // For simplicity here, we assume it's handled or done elsewhere if needed frequently.
    // Exiv2::XmpProperties::registerNs(ns_uri, ns_prefix); // Could cause issues if called repeatedly or concurrently

    Exiv2::XmpData xmp_data;

    try {
        // Add metadata about the serialization itself
        xmp_data["Xmp.cm.serializedBy"] = "CaptureMoment";
        xmp_data["Xmp.cm.version"] = "1.0"; // Your app version
        // Include the source image path as a metadata field within the XMP
        xmp_data["Xmp.cm.sourceImagePath"] = source_image_path.data();

        // Iterate through operations with index using C++23 std::views::enumerate
        for (auto [index, const& op] : std::views::enumerate(operations))
        {
            // 'index' is automatically a std::size_t (or similar unsigned integral type)
            // 'op' is a const reference to the current OperationDescriptor
            std::string index_str = std::to_string(index + 1); // Start indexing from 1 for readability in XMP

            // Use magic_enum directly here
            std::string_view type_name_view = magic_enum::enum_name(op.type);
            xmp_data["Xmp.cm.operation[" + index_str + "].type"] = type_name_view.data();
            xmp_data["Xmp.cm.operation[" + index_str + "].name"] = op.name.c_str();
            xmp_data["Xmp.cm.operation[" + index_str + "].enabled"] = op.enabled;

            // Iterate through the generic parameters map using range-for
            for (const auto& [param_name, param_value] : op.params) {
                std::string xmp_param_key = "Xmp.cm.operation[" + index_str + "].param." + param_name;
                std::string param_str_value = serializeAnyParameter(param_value);
                if (!param_str_value.empty()) { // Only add if serialization was successful
                     xmp_data[xmp_param_key] = param_str_value.c_str();
                } else {
                     spdlog::warn("FileSerializerWriter::serializeOperationsToXmp: Could not serialize parameter '{}' for operation '{}' (index {}). Skipping.", param_name, op.name, index);
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
}

std::string FileSerializerWriter::serializeAnyParameter(const std::any& value) const
{
    // This function needs to handle different types stored in std::any.
    // You need to add support for all the types you expect to store.
    // Example: float, int, bool, std::string, etc.

    const std::type_info& type = value.type();

    if (value.type() == typeid(float)) {
        return std::to_string(std::any_cast<float>(value));
    } else if (value.type() == typeid(double)) {
        return std::to_string(std::any_cast<double>(value));
    } else if (value.type() == typeid(int)) {
        return std::to_string(std::any_cast<int>(value));
    } else if (value.type() == typeid(bool)) {
        return std::any_cast<bool>(value) ? "true" : "false";
    } else if (value.type() == typeid(std::string)) {
        return std::any_cast<std::string>(value);
    }
    // Add more types as needed...

    // If the type is not supported, return an empty string
    spdlog::warn("FileSerializerWriter::serializeAnyParameter: Unsupported type for serialization: {}", value.type().name());
    return {};
}

} // namespace CaptureMoment::Core::Serializer
