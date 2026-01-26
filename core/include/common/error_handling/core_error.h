/**
 * @file core_error.h
 * @brief Central error handling system for CaptureMoment Core library.
 *
 * @details
 * This header provides global error definitions and categories for CaptureMoment Core.
 *
 * It centralizes how errors are defined and described for the logger (spdlog).
 *
 * **Architecture:**
 * 1. **`CoreError`**: Enum containing domain-specific error codes (InvalidImageRegion, AllocationFailed).
 * 2. **`CoreErrorCategory`**: Categories for filtering logs.
 *
 * **Integration:**
 * Include this header in `core/pch.h` for global availability.
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include <string_view>
#include <Halide.h>
#include <include "config/app_config.h" // Pour m_backend
#include <spdlog/spdlog.h>

namespace CaptureMoment::Core {

namespace ErrorHandling {

/**
 * @brief Enumeration of all error codes for CaptureMoment Core.
 *
 * @details
 * This enum centralizes error codes for different subsystems (Image Processing, Serialization).
 * Using an enum allows static type checking and human-readable string conversion.
 */
enum class CoreError {
    /**
     * @brief No error.
     */
    Success = 0,

    /**
     * @brief The provided ImageRegion dimensions or data are invalid.
     */
    InvalidImageRegion = 1,

    /**
     * @brief Memory allocation failed (bad_alloc).
     */
    AllocationFailed = 2,

    /**
     * @brief Halide buffer is invalid or undefined.
     */
    InvalidHalideBuffer = 3,

    /**
     * @brief File not found or permission denied (I/O error).
     */
    IOError = 4,

    /**
     * @brief An unexpected error occurred.
     */
    Unexpected = 99
};

/**
 * @brief Enumeration of error categories for robust logging.
 *
 * @details
 * This enum maps `CoreError` codes to high-level category names (System, GPU, Serialization).
 * It ensures that critical errors are easily distinguishable in the logs.
 */
enum class CoreErrorCategory {
    /**
     * @brief General system or unknown errors.
     */
    System = 0,

    /**
     * @brief Errors related to Image Processing.
     */
    ImageProcessing = 1,

    /**
     * @brief Errors related to Serialization or Hardware abstraction.
     */
    Serialization = 2
};

/**
 * @brief Converts `CoreError` code to a human-readable category string.
 *
 * @details
 * This helper function isolates the logic of string mapping from `CoreError`.
 *
 * @param code The error code.
 * @return A string_view of the category name (e.g., "ImageProcessing", "System").
 */
[[nodiscard]] std::string_view core_error_to_category(CoreError code)
{
    switch (code)
    {
    case CoreError::Success:
        return "System";

    case CoreError::InvalidImageRegion:
        return "ImageProcessing";

    case CoreError::AllocationFailed:
        return "ImageProcessing";

    case CoreError::InvalidHalideBuffer:
        return "ImageProcessing";

    case CoreError::IOError:
        return "Serialization";

    case CoreError::Unexpected:
        return "System";

    default:
        return "Unknown";
    }
}

// ============================================================
// Helpers
// ============================================================

/**
 * @brief Returns the memory type corresponding to a specific error code.
 *
 * @param code The `CoreError` enum value.
 * @return MemoryType.
 */
[[nodiscard]] Common::MemoryType error_to_memory_type(CoreError code)
{
    switch (code)
    {
    case CoreError::AllocationFailed:
        return Common::MemoryType::CPU_RAM;

    case CoreError::InvalidHalideBuffer:
        return Common::MemoryType::GPU_MEMORY;

    default:
        return Common::MemoryType::CPU_RAM; // Default safe fallback
    }
}

} // namespace ErrorHandling

} // namespace CaptureMoment::Core
