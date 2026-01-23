/**
 * @file operation_contrast.h
 * @brief Concrete implementation of Contrast adjustment
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
 * @class OperationContrast
 * @brief Adjusts the contrast of an image region.
 *
 * This operation modifies the contrast by scaling the difference from the midpoint (0.5).
 * Higher values increase contrast, lower values decrease contrast.
 *
 * **Algorithm**:
 * For each pixel `p` and channel `c` (excluding alpha):
 * \f$ p_c = 0.5 + (p_c - 0.5) \times \text{factor} \f$
 *
 * **Parameters**:
 * - `value` (float): The contrast adjustment factor.
 * - Range: Defined by OperationRanges::getContrastMinValue() and OperationRanges::getContrastMaxValue()
 * - Default: OperationRanges::getContrastDefaultValue() (typically 1.0f, No change)
 * - > 1.0: Increase contrast
 * - < 1.0: Decrease contrast
 */
class OperationContrast : public IOperation,  public IOperationFusionLogic
{
public:
    // --- Metadata ---
    [[nodiscard]] OperationType type() const override { return OperationType::Contrast; }
    [[nodiscard]] const char* name() const override { return "Contrast"; }

    // --- Range Access (via the centralized ranges) ---
    /**
     * @brief Minimum allowed contrast value.
     * Defined by OperationRanges::getContrastMinValue().
     */
    static constexpr float MIN_CONTRAST_VALUE = OperationRanges::getContrastMinValue();

    /**
     * @brief Maximum allowed contrast value.
     * Defined by OperationRanges::getContrastMaxValue().
     */
    static constexpr float MAX_CONTRAST_VALUE = OperationRanges::getContrastMaxValue();

    /**
     * @brief Default contrast value.
     * Defined by OperationRanges::getContrastDefaultValue().
     */
    static constexpr float DEFAULT_CONTRAST_VALUE = OperationRanges::getContrastDefaultValue();

    /**
     * @brief Applies the contrast adjustment.
     *
     * This method provides sequential execution capability for the whites adjustment operation.
     * While primarily replaced by the fused pipeline system (appendToFusedPipeline), it remains
     * available for specific use cases such as debugging, testing, or standalone operation execution.
     *
     * Reads the "value" parameter from the descriptor and applies the contrast
     * formula to every color channel (RGB) of every pixel in the working image.
     * The alpha channel is left unchanged.
     * Performs a validation check to ensure the value is within the defined range [MIN_CONTRAST_VALUE, MAX_CONTRAST_VALUE].
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
     * This method implements the fusion logic specific to the Contrast adjustment,
     * applying the adjustment without intermediate memory allocations, directly within the fused pipeline.
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
     *               contrast adjustment value and other relevant parameters.
     * @return A new Halide::Func representing the output of this operation,
     *         which can be used as input for the next operation in the fused pipeline.
     *         The returned function encapsulates the logic to adjust the contrast
     *         operating directly on the pixel data stream using the shared coordinate variables.
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
