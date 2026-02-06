/**
 * @file serializer.h
 * @brief Umbrella header for the Serializer module.
 *
 * This header provides a single entry point for the `CaptureMoment::Core::Serializer` namespace.
 * Including this file grants access to all interfaces and concrete implementations
 * related to XMP metadata handling, file serialization, and path strategies.
 *
 * This module is responsible for:
 * - **Metadata**: Reading and writing XMP metadata using Exiv2.
 * - **Strategies**: Choosing where to store metadata (sidecar, appdata, embedded).
 * - **Serialization**: Saving/loading operation parameters and state to/from disk.
 * - **Error Handling**: Using `std::expected` and `CoreError` for robust error reporting.
 *
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

// ============================================================
// 1. Interfaces
// ============================================================

/**
 * @brief Interface for XMP metadata providers (e.g., Exiv2).
 */
#include "serializer/i_xmp_provider.h"

/**
 * @brief Interface for determining XMP file paths (e.g., sidecar, appdata).
 */
#include "serializer/i_xmp_path_strategy.h"

/**
 * @brief Interface for reading serialized data from files.
 */
#include "serializer/i_file_serializer_reader.h"

/**
 * @brief Interface for writing serialized data to files.
 */
#include "serializer/i_file_serializer_writer.h"

// ============================================================
// 2. Providers (Exiv2)
// ============================================================

/**
 * @brief Initializes the Exiv2 library globally.
 */
#include "serializer/provider/exiv2_initializer.h"

/**
 * @brief Concrete XMP provider using Exiv2.
 */
#include "serializer/provider/exiv2_provider.h"

// ============================================================
// 3. Strategies
// ============================================================

/**
 * @brief Strategy for storing XMP in application data directory.
 */
#include "serializer/strategy/appdata_xmp_path_strategy.h"

/**
 * @brief Configurable strategy for XMP path resolution.
 */
#include "serializer/strategy/configurable_xmp_path_strategy.h"

/**
 * @brief Strategy for storing XMP in sidecar files.
 */
#include "serializer/strategy/sidecar_xmp_path_strategy.h"

// ============================================================
// 4. Concrete Implementations (Readers/Writers)
// ============================================================

/**
 * @brief Concrete implementation for reading serialized data.
 */
#include "serializer/file_serializer_reader.h"

/**
 * @brief Concrete implementation for writing serialized data.
 */
#include "serializer/file_serializer_writer.h"

// ============================================================
// 5. Manager
// ============================================================

/**
 * @brief Main manager for coordinating serialization tasks.
 */
#include "serializer/file_serializer_manager.h"


// ============================================================
// 6. Operations
// ============================================================

/**
 * @brief Operations for serializing and deserializing pipeline states.
 */
#include "serializer/operation_serialization.h"