/**
 * @file operation_whites.h
 * @brief Concrete implementation of Whites adjustment
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once
#include "operations/interfaces/i_operation.h"
#include "operations/interfaces/i_operation_fusion_logic.h"
#include "operations/interfaces/i_operation_default_logic.h"
#include "operations/operation_ranges.h"

namespace CaptureMoment::Core {

namespace Operations {
/**
 * @class OperationWhites
 * @brief Adjusts the white point of an image region.
 *
 * This operation modifies the luminance of the brightest areas of the image,
 * effectively shifting the white point.
 * Increasing whites brightens the overall image and makes whites more bright.
 * Decreasing whites darkens the overall image and makes whites less bright.
 *
 * **Algorithm** (Approximation - actual implementations can be more complex):
 * For each pixel `p` and channel `c` (excluding alpha), if the luminance is above a high threshold:
 * \f$ p_c = p_c + \text{value} \times \text{adjustment_factor} \f$
 * This is a simplified version focusing on the upper part of the luminance range.
 *
 * **Parameters**:
 * - `value` (float): The whites adjustment factor.
 * - Range: Defined by OperationRanges::getWhitesMinValue() and OperationRanges::getWhitesMaxValue()
 * - Default: OperationRanges::getWhitesDefaultValue() (typically 0.0f, No change)
 * - > 0: Brighten whites (make them more bright)
 * - < 0: Darken whites (make them less bright)
 */
class OperationWhites : public IOperation,  public IOperationFusionLogic, public IOperationDefaultLogic
{
public:
    // --- Metadata ---
    [[nodiscard]] OperationType type() const override { return OperationType::Whites; }
    [[nodiscard]] const char* name() const override { return "Whites"; }

    // --- Range Access (via the centralized ranges) ---
    /**
     * @brief Minimum allowed whites value.
     * Defined by OperationRanges::getWhitesMinValue().
     */
    static constexpr float MIN_WHITES_VALUE = OperationRanges::getWhitesMinValue();

    /**
     * @brief Maximum allowed whites value.
     * Defined by OperationRanges::getWhitesMaxValue().
     */
    static constexpr float MAX_WHITES_VALUE = OperationRanges::getWhitesMaxValue();

    /**
     * @brief Default whites value.
     * Defined by OperationRanges::getWhitesDefaultValue().
     */
    static constexpr float DEFAULT_WHITES_VALUE = OperationRanges::getWhitesDefaultValue();

    /**
     * @brief Applies the whites adjustment.
     *
     * This method provides sequential execution capability for the whites adjustment operation.
     * While primarily replaced by the fused pipeline system (appendToFusedPipeline), it remains
     * available for specific use cases such as debugging, testing, or standalone operation execution.
     *
     * Reads the "value" parameter from the descriptor and applies the whites
     * formula to every color channel (RGB) of every pixel in the working image,
     * primarily affecting pixels with very high luminance (the "whites").
     * The alpha channel is left unchanged.
     * Performs a validation check to ensure the value is within the defined range [MIN_WHITES_VALUE, MAX_WHITES_VALUE].
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
     * This method implements the fusion logic specific to the Whites adjustment,
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
     *               white level adjustment value and other relevant parameters.
     * @return A new Halide::Func representing the output of this operation,
     *         which can be used as input for the next operation in the fused pipeline.
     *         The returned function encapsulates the logic to adjust the white levels
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


    /**
     * @brief Executes the adjustment on a raw ImageRegion (CPU fallback).
     */
    [[nodiscard]] bool executeOnImageRegion(
        Common::ImageRegion& region,
        const OperationDescriptor& params
        ) const override;
};

} // namespace Operations

} // namespace CaptureMoment::Core
