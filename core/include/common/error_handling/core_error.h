/**
 * @file core_error.h
 * @brief Central error handling system for CaptureMoment Core.
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <system_error>
#include <string>

namespace CaptureMoment::Core {

namespace ErrorHandling {
/**
 * @brief Enumeration of all specific error codes for the CaptureMoment Core library.
 *
 * Using an enum allows static type checking, while the std::error_category
 * allows human-readable string generation and cross-system error compatibility.
 */
enum class CoreError {
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
     * @brief Halide buffer validation failed.
     */
    InvalidHalideBuffer = 3,

    /**
     * @brief Operation attempted on an uninitialized or invalid working image.
     */
    InvalidWorkingImage = 4,
    
    /**
     * @brief Generic I/O error (File not found, read error, etc.).
     */
    IOError = 5
};

/**
 * @brief std::error_category specialization for CoreError.
 *
 * This class defines how CoreError is converted to the std::error_code system.
 * It provides the default error message string for each error code.
 */
class CoreErrorCategory : public std::error_category {
public:
    /**
     * @brief Returns the name of the category ("CoreError").
     */
    [[nodiscard]] const char* name() const noexcept override {
        return "CaptureMomentCoreError";
    }

    /**
     * @brief Returns a human-readable description of the error code.
     * @param ev The error value (int representation of CoreError).
     * @return A string describing the error.
     */
    [[nodiscard]] std::string message(int ev) const override {
        switch (static_cast<CoreError>(ev)) {
            case CoreError::Success:          return "Success";
            case CoreError::InvalidImageRegion: return "Provided ImageRegion is invalid (dimensions or data mismatch)";
            case CoreError::AllocationFailed:  return "Memory allocation failed (out of memory)";
            case CoreError::InvalidHalideBuffer: return "Halide buffer is undefined or invalid";
            case CoreError::InvalidWorkingImage: return "Working image is in invalid state";
            case CoreError::IOError:          return "Input/Output operation failed";
            default:                           return "Unknown CaptureMoment Core Error";
        }
    }
};

/**
 * @brief Global instance of the category.
 * Standard pattern for std::error_code integration.
 */
inline const CoreErrorCategory& getCoreErrorCategory() {
    static CoreErrorCategory instance;
    return instance;
}

/**
 * @brief Enables automatic conversion from CoreError to std::error_code.
 * This allows returning CoreError directly where std::error_code is expected.
 */
inline std::error_code make_error_code(CoreError e) {
    return std::error_code(static_cast<int>(e), getCoreErrorCategory());
}

} // namespace ErrorHandling

} // namespace CaptureMoment::Core

// ============================================================================
// Standard Library Specialization
// Allows is_error_code_enum<CoreError> to be true.
// ============================================================================
template<>
struct std::is_error_code_enum<CaptureMoment::Core::ErrorHandling::CoreError> : std::true_type {};
