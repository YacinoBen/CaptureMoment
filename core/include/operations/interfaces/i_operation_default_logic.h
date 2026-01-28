/**
 * @file i_operation_default_logic.h
 * @brief Interface for providing the default (CPU) execution logic of an operation.
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include "operations/operation_descriptor.h"
#include "common/image_region.h"
#include <expected>
#include "common/error_handling/core_error.h"

namespace CaptureMoment::Core {

namespace Operations {

/**
 * @interface IOperationDefaultLogic
 * @brief Interface for providing the default (CPU) execution logic of an operation.
 *
 * Each operation that supports generic or fallback execution should provide an implementation
 * of this interface. It allows fallback executors (like FallbackPipelineExecutor)
 * to apply the operation logic directly to raw image data (ImageRegion).
 * This provides a baseline execution path when optimized pipelines (e.g., Halide fusion)
 * are not available or suitable.
 *
 * @par Error Handling
 * Consistent with IOperation, this method returns `std::expected<void, CoreError>`.
 */
class IOperationDefaultLogic {
public:
    virtual ~IOperationDefaultLogic() = default;

    /**
     * @brief Executes this operation's logic on a raw ImageRegion (CPU fallback).
     *
     * @details
     * This method applies the operation's specific algorithm directly to the pixel data
     * contained within the provided ImageRegion. It modifies the region in-place.
     *
     * @param region The ImageRegion containing the pixel data to modify.
     * @param params The configuration/settings for this execution.
     * @return std::expected<void, CoreError>
     *         Returns void on success, or a specific CoreError code on failure.
     */
    [[nodiscard]] virtual std::expected<void, ErrorHandling::CoreError> executeOnImageRegion(
        Common::ImageRegion& region,
        const OperationDescriptor& params
        ) const = 0;
};

} // namespace Operations

} // namespace CaptureMoment::Core
