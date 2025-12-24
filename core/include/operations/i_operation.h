/**
 * @file i_operation.h
 * @brief Interface for image processing operations
 * @author CaptureMoment Team
 * @date 2025
 */
#pragma once

#include "operations/operation_type.h"
#include "operations/operation_descriptor.h"
#include "common/image_region.h"

namespace CaptureMoment::Core {

namespace Operations {
/**
 * @interface IOperation
 * @brief Abstract base class for all image processing algorithms.
 *
 * * Every image effect (Brightness, Contrast, etc.) must implement this interface.
 * It provides a standardized way for the processing pipeline to execute
 * operations without knowing their specific implementation details.
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
    
    /**
     * @brief Executes the operation on an image region.
     * * This is the core method where the image processing logic resides.
     * The operation should read parameters from @p params and modify @p input in-place.
     * @param input The image buffer to modify (in-place).
     * @param params The configuration/settings for this execution.
     * @return true if execution succeeded, false otherwise.
     */
    [[nodiscard]] virtual bool execute(Common::ImageRegion& input, const OperationDescriptor& params) = 0;

    /**
     * @brief Indicates if this operation supports GPU acceleration (e.g., via Halide).
     * @return true if GPU implementation is available.
     */
    [[nodiscard]] virtual bool canRunOnGPU() const { return false; }

    /**
     * @brief Indicates if this operation is thread-safe.
     * If true, multiple threads can call execute() on different regions simultaneously.
     * @return true by default.
     */
    [[nodiscard]] virtual bool isThreadSafe() const { return true; }
};

} // namespace Operations

} // namespace CaptureMoment::Core
