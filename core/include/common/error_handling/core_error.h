/**
 * @file core_error.h
 * @brief Central error handling system for CaptureMoment Core library.
 * @details Defines error codes and categories using memory-efficient enums.
 */

#pragma once

#include <string_view>
#include <cstdint>

namespace CaptureMoment::Core {

namespace ErrorHandling {

/**
 * @brief Enumeration of all error codes for CaptureMoment Core.
 * @details Inherits from uint8_t to minimize memory footprint (1 byte instead of 4).
 */
enum class CoreError : uint8_t {
    // ==========================================
    // Success State
    // ==========================================

    /**
     * @brief Operation completed successfully.
     */
    Success = 0,

    // ==========================================
    // Image Processing Errors
    // ==========================================

    /**
     * @brief The provided ImageRegion dimensions or data are invalid.
     * @details Typically raised when width <= 0, height <= 0, or bounds are out of range.
     */
    InvalidImageRegion = 1,

    /**
     * @brief Memory allocation failed.
     * @details Raised when a CPU (std::bad_alloc) or GPU allocation fails during buffer creation.
     */
    AllocationFailed = 2,

    /**
     * @brief Halide buffer is invalid or undefined.
     * @details The underlying buffer handle is null or improperly initialized.
     */
    InvalidHalideBuffer = 3,

    /**
     * @brief The working image state is invalid or corrupted.
     * @details Raised when the internal working image data cannot be processed or is missing.
     */
    InvalidWorkingImage = 4,

    // ==========================================
    // I/O Errors
    // ==========================================

    /**
     * @brief File not found or permission denied.
     * @details Covers file system level errors (read/write).
     */
    IOError = 5,

    // ==========================================
    // System Errors
    // ==========================================

    /**
     * @brief An unexpected error occurred.
     * @details Catch-all for logic errors or undefined states.
     */
    Unexpected = 99
};

/**
 * @brief Enumeration of error categories.
 * @details Used for filtering logs or dispatching recovery strategies.
 */
enum class CoreErrorCategory : uint8_t {
    /**
     * @brief General system or unknown errors.
     */
    System = 0,

    /**
     * @brief Errors related to Image Processing, GPU, or Memory.
     */
    ImageProcessing = 1,

    /**
     * @brief Errors related to Serialization, File I/O, or Network.
     */
    Serialization = 2
};


// ============================================================
// Helpers (Conversion functions)
// ============================================================

/**
 * @brief Converts a CoreError code to its high-level Category.
 *
 * @param code The error code to categorize.
 * @return CoreErrorCategory The corresponding category.
 */
[[nodiscard]] constexpr CoreErrorCategory get_error_category(CoreError code)
{
    switch (code)
    {
    case CoreError::InvalidImageRegion:
    case CoreError::AllocationFailed:
    case CoreError::InvalidHalideBuffer:
    case CoreError::InvalidWorkingImage:
        return CoreErrorCategory::ImageProcessing;

    case CoreError::IOError:
        return CoreErrorCategory::Serialization;

    case CoreError::Success:
    case CoreError::Unexpected:
    default:
        return CoreErrorCategory::System;
    }
}

/**
 * @brief Converts a CoreError code to a human-readable string.
 * @note Marked constexpr for potential compile-time optimization, but callable at runtime.
 *
 * @param code The error code.
 * @return std::string_view String representation of the error.
 */
[[nodiscard]] constexpr std::string_view to_string(CoreError code)
{
    switch (code)
    {
    case CoreError::Success: return "Success";
    case CoreError::InvalidImageRegion: return "InvalidImageRegion";
    case CoreError::AllocationFailed: return "AllocationFailed";
    case CoreError::InvalidHalideBuffer: return "InvalidHalideBuffer";
    case CoreError::InvalidWorkingImage: return "InvalidWorkingImage"; // Ajouté ici
    case CoreError::IOError: return "IOError";
    case CoreError::Unexpected: return "Unexpected";
    default: return "Unknown";
    }
}

/**
 * @brief Converts a CoreErrorCategory to a human-readable string.
 *
 * @param category The error category.
 * @return std::string_view String representation of the category.
 */
[[nodiscard]] constexpr std::string_view to_string(CoreErrorCategory category)
{
    switch (category)
    {
    case CoreErrorCategory::System: return "System";
    case CoreErrorCategory::ImageProcessing: return "ImageProcessing";
    case CoreErrorCategory::Serialization: return "Serialization";
    default: return "Unknown";
    }
}

} // namespace ErrorHandling

} // namespace CaptureMoment::Core
