/**
 * @file operation_shadows.h
 * @brief Concrete implementation of Shadows adjustment
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once
#include "operations/interfaces/i_operation.h"
#include "operations/interfaces/i_operation_fusion_logic.h"
#include "operations/operation_ranges.h"

namespace CaptureMoment::Core {

namespace Operations {
/**
 * @class OperationShadows
 * @brief Adjusts the shadow tones of an image region.
 *
 * This operation modifies the luminance of the darkest areas of the image,
 * effectively shifting the black point.
 * Increasing shadows brightens the overall image and makes shadows less dark.
 * Decreasing shadows darkens the overall image and makes shadows darker.
 *
 * **Algorithm** (Approximation - actual implementations can be more complex):
 * For each pixel `p` and channel `c` (excluding alpha), if the luminance is below a low threshold:
 * \f$ p_c = p_c + \text{value} \times \text{adjustment_factor} \f$
 * This is a simplified version focusing on the lower part of the luminance range.
 *
 * **Parameters**:
 * - `value` (float): The shadows adjustment factor.
 * - Range: Defined by OperationRanges::getShadowsMinValue() and OperationRanges::getShadowsMaxValue()
 * - Default: OperationRanges::getShadowsDefaultValue() (typically 0.0f, No change)
 * - > 0: Brighten shadows (make them less dark)
 * - < 0: Darken shadows (make them more dark)
 */
class OperationShadows : public IOperation,  public IOperationFusionLogic
{
public:
    // --- Metadata ---
    [[nodiscard]] OperationType type() const override { return OperationType::Shadows; }
    [[nodiscard]] const char* name() const override { return "Shadows"; }

    // --- Range Access (via the centralized ranges) ---
    /**
     * @brief Minimum allowed shadows value.
     * Defined by OperationRanges::getShadowsMinValue().
     */
    static constexpr float MIN_SHADOWS_VALUE = OperationRanges::getShadowsMinValue();

    /**
     * @brief Maximum allowed shadows value.
     * Defined by OperationRanges::getShadowsMaxValue().
     */
    static constexpr float MAX_SHADOWS_VALUE = OperationRanges::getShadowsMaxValue();

    /**
     * @brief Default shadows value.
     * Defined by OperationRanges::getShadowsDefaultValue().
     */
    static constexpr float DEFAULT_SHADOWS_VALUE = OperationRanges::getShadowsDefaultValue();

    /**
     * @brief Applies the shadows adjustment.
     *
     * This method provides sequential execution capability for the whites adjustment operation.
     * While primarily replaced by the fused pipeline system (appendToFusedPipeline), it remains
     * available for specific use cases such as debugging, testing, or standalone operation execution.
     *
     * Reads the "value" parameter from the descriptor and applies the shadows
     * formula to every color channel (RGB) of every pixel in the working image,
     * primarily affecting pixels with very low luminance (the "shadows").
     * The alpha channel is left unchanged.
     * Performs a validation check to ensure the value is within the defined range [MIN_SHADOWS_VALUE, MAX_SHADOWS_VALUE].
     * @param working_image The hardware-agnostic image buffer to modify.
     * @param params Must contain a "value" (float) parameter.
     * @return true if successful.
     */
    [[maybe_unused]] [[nodiscard]] bool execute(ImageProcessing::IWorkingImageHardware& working_image, const OperationDescriptor& params) override;

    /**
     * @brief Appends this operation's logic to a fused Halide pipeline.
     * This method is used by the PipelineBuilder to combine multiple operations
     * into a single computational pass. It takes an input function and returns
     * a new function representing the current operation applied to the input.
     * This method implements the fusion logic specific to the Shadows adjustment,
     * calculating luminance-based masks and applying the adjustment without
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
     *               shadow level adjustment value and other relevant parameters.
     * @return A new Halide::Func representing the output of this operation,
     *         which can be used as input for the next operation in the fused pipeline.
     *         The returned function encapsulates the logic to adjust the shadow levels
     *         based on luminance masking, operating directly on the pixel data stream
     *         using the shared coordinate variables.
     */
    [[nodiscard]] Halide::Func appendToFusedPipeline(
        const Halide::Func& input_func,
        const Halide::Var& x,
        const Halide::Var& y,
        const Halide::Var& c,
        const OperationDescriptor& params
        ) const override;
};

} // namespace Operations

} // namespace CaptureMoment::Core
