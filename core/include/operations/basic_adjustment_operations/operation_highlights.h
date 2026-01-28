/**
 * @file operation_highlights.h
 * @brief Concrete implementation of Highlights adjustment
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
 * @class OperationHighlights
 * @brief Adjusts the highlight tones of an image region.
 *
 * This operation modifies the luminance of the brightest areas of the image,
 * effectively shifting the white point.
 * Increasing highlights darkens the overall image and makes whites less bright.
 * Decreasing highlights brightens the overall image and makes whites brighter.
 *
 * **Algorithm** (Approximation - actual implementations can be more complex):
 * For each pixel `p` and channel `c` (excluding alpha), if the luminance is above a high threshold:
 * \f$ p_c = p_c + \text{value} \times \text{adjustment_factor} \f$
 * This is a simplified version focusing on the upper part of the luminance range.
 *
 * **Parameters**:
 * - `value` (float): The highlights adjustment factor.
 * - Range: Defined by OperationRanges::getHighlightsMinValue() and OperationRanges::getHighlightsMaxValue()
 * - Default: OperationRanges::getHighlightsDefaultValue() (typically 0.0f, No change)
 * - > 0: Darken highlights (make them less bright)
 * - < 0: Brighten highlights (make them more bright)
 */
class OperationHighlights : public IOperation,  public IOperationFusionLogic, public IOperationDefaultLogic
{
public:
    // --- Metadata ---
    [[nodiscard]] OperationType type() const override { return OperationType::Highlights; }
    [[nodiscard]] const char* name() const override { return "Highlights"; }

    // --- Range Access (via the centralized ranges) ---
    /**
     * @brief Minimum allowed highlights value.
     * Defined by OperationRanges::getHighlightsMinValue().
     */
    static constexpr float MIN_HIGHLIGHTS_VALUE = OperationRanges::getHighlightsMinValue();

    /**
     * @brief Maximum allowed highlights value.
     * Defined by OperationRanges::getHighlightsMaxValue().
     */
    static constexpr float MAX_HIGHLIGHTS_VALUE = OperationRanges::getHighlightsMaxValue();

    /**
     * @brief Default highlights value.
     * Defined by OperationRanges::getHighlightsDefaultValue().
     */
    static constexpr float DEFAULT_HIGHLIGHTS_VALUE = OperationRanges::getHighlightsDefaultValue();

    /**
     * @brief Applies the highlights adjustment.
     *
     * This method provides sequential execution capability for the whites adjustment operation.
     * While primarily replaced by the fused pipeline system (appendToFusedPipeline), it remains
     * available for specific use cases such as debugging, testing, or standalone operation execution.
     *
     * Reads the "value" parameter from the descriptor and applies the highlights
     * formula to every color channel (RGB) of every pixel in the working image,
     * primarily affecting pixels with very high luminance (the "highlights").
     * The alpha channel is left unchanged.
     * Performs a validation check to ensure the value is within the defined range [MIN_HIGHLIGHTS_VALUE, MAX_HIGHLIGHTS_VALUE].
     * @param working_image The hardware-agnostic image buffer to modify.
     * @param params Must contain a "value" (float) parameter.
     * @return std::expected<void, ErrorHandling::CoreError>.
     */
    [[maybe_unused]] [[nodiscard]] std::expected<void, ErrorHandling::CoreError> execute(ImageProcessing::IWorkingImageHardware& working_image, const OperationDescriptor& params) override;

    /**
     * @brief Appends this operation's logic to a fused Halide pipeline.
     * This method is used by the PipelineBuilder to combine multiple operations
     * into a single computational pass. It takes an input function and returns
     * a new function representing the current operation applied to the input.
     * This method implements the fusion logic specific to the Highlights adjustment,
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
     *               highlight level adjustment value and other relevant parameters.
     * @return A new Halide::Func representing the output of this operation,
     *         which can be used as input for the next operation in the fused pipeline.
     *         The returned function encapsulates the logic to adjust the highlight levels
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
    [[nodiscard]] std::expected<void, ErrorHandling::CoreError> executeOnImageRegion(
        Common::ImageRegion& region,
        const OperationDescriptor& params
        ) const override;
};

} // namespace Operations

} // namespace CaptureMoment::Core
