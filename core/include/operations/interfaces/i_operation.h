/**
 * @file i_operation.h
 * @brief Interface for image processing operations
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include "operations/operation_type.h"
#include "operations/operation_descriptor.h"
#include "image_processing/interfaces/i_working_image_hardware.h"
#include <expected>
#include "common/error_handling/core_error.h"

namespace CaptureMoment::Core {

namespace Operations {

/**
 * @interface IOperation
 * @brief Abstract base class for all image processing algorithms.
 *
 * Every image effect (Brightness, Contrast, etc.) must implement this interface.
 * It provides a standardized way for the processing pipeline to execute
 * operations without knowing their specific implementation details.
 *
 * The core execute method operates on a hardware-agnostic IWorkingImageHardware,
 * enabling seamless CPU/GPU backend switching.
 *
 * @par Error Handling
 * The execute method returns `std::expected<void, CoreError>`.
 * Operations must return an error code (e.g., InvalidWorkingImage, AllocationFailed)
 * if the operation cannot be performed, rather than returning a simple false boolean.
 */
class IOperation {
public:
    virtual ~IOperation() = default;

    /**
     * @brief Gets the unique type identifier of the operation.
     * @return The OperationType enum value.
     */
    [[nodiscard]] virtual OperationType type() const = 0;

    /**
     * @brief Gets the constant name of the operation.
     * @return A C-string identifier (e.g., "Brightness").
     */
    [[nodiscard]] virtual const char* name() const = 0;

};

} // namespace Operations

} // namespace CaptureMoment::Core
