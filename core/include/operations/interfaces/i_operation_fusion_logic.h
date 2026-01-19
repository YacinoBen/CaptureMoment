/**
 * @file i_operation_fusion_logic.h
 * @brief Interface for providing the Halide fusion logic of an operation.
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include <Halide.h>
#include "operations/operation_descriptor.h"

namespace CaptureMoment::Core {

namespace Operations {

/**
 * @interface IOperationFusionLogic
 * @brief Interface for providing the Halide fusion logic of an operation.
 * 
 * Each operation that supports pipeline fusion should provide an implementation
 * of this interface. It allows the PipelineBuilder to combine multiple operations
 * into a single computational pass for improved performance.
 */
class IOperationFusionLogic {
public:
    virtual ~IOperationFusionLogic() = default;

    /**
     * @brief Appends this operation's logic to a fused Halide pipeline.
     * This method is used by the PipelineBuilder to combine multiple operations
     * into a single computational pass. It takes an input function and returns
     * a new function representing the current operation applied to the input.
     * @param input_func The Halide function representing the input to this operation.
     * @param params The configuration/settings for this operation.
     * @return A new Halide::Func representing the output of this operation.
     */
    [[nodiscard]] virtual Halide::Func appendToFusedPipeline(
        const Halide::Func& input_func,
        const OperationDescriptor& params
    ) const = 0;
};

} // namespace Operations

} // namespace CaptureMoment::Core
