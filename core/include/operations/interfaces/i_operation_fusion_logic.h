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
     * This method implements the fusion logic specific to the operation,
     * calculating transformations and applying adjustments without
     * intermediate memory allocations, directly within the fused pipeline.
     * All operations in the fused pipeline must use the same coordinate variables
     * (x, y, c) to ensure consistency and proper chaining of operations.
     * @param input_func The Halide function representing the input to this operation.
     *                   This function contains the image data from the previous
     *                   operation in the pipeline or the original image if this is the first operation.
     * @param x The Halide variable for the x dimension, shared across all operations
     *          in the fused pipeline to ensure coordinate consistency.
     * @param y The Halide variable for the y dimension, shared across all operations
     *          in the fused pipeline to ensure coordinate consistency.
     * @param c The Halide variable for the channel dimension, shared across all operations
     *          in the fused pipeline to ensure coordinate consistency.
     * @param params The configuration/settings for this operation, containing the
     *               adjustment values and other relevant parameters.
     * @return A new Halide::Func representing the output of this operation,
     *         which can be used as input for the next operation in the fused pipeline.
     *         The returned function encapsulates the operation's logic,
     *         operating directly on the pixel data stream using the shared coordinate variables.
     */
    [[nodiscard]] virtual Halide::Func appendToFusedPipeline(
        const Halide::Func& input_func,
        const Halide::Var& x,
        const Halide::Var& y,
        const Halide::Var& c,
        const OperationDescriptor& params
    ) const = 0;
};

} // namespace Operations

} // namespace CaptureMoment::Core
