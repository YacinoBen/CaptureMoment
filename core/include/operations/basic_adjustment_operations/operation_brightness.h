/**
 * @file operation_brightness.h
 * @brief Concrete implementation of Brightness adjustment
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
 * @class OperationBrightness
 * @brief Adjusts the brightness of an image region.
 *
 * This operation modifies the overall luminance of the image by adding a constant value to each pixel.
 * Positive values increase brightness, negative values decrease brightness.
 *
 * **Algorithm**:
 * For each pixel `p` and channel `c` (excluding alpha):
 * \f$ p_c = p_c + \text{value} \f$
 *
 * **Parameters**:
 * - `value` (float): The brightness adjustment factor.
 * - Range: Defined by OperationRanges::getBrightnessMinValue() and OperationRanges::getBrightnessMaxValue()
 * - Default: OperationRanges::getBrightnessDefaultValue() (typically 0.0f, No change)
 * - > 0: Increase brightness
 * - < 0: Decrease brightness
 */
class OperationBrightness : public IOperation,  public IOperationFusionLogic, public IOperationDefaultLogic
{
public:
    // --- Metadata ---
    [[nodiscard]] OperationType type() const override { return OperationType::Brightness; }
    [[nodiscard]] const char* name() const override { return "Brightness"; }

    // --- Range Access (via the centralized ranges) ---
    /**
     * @brief Minimum allowed brightness value.
     * Defined by OperationRanges::getBrightnessMinValue().
     */
    static constexpr float MIN_BRIGHTNESS_VALUE = OperationRanges::getBrightnessMinValue();

    /**
     * @brief Maximum allowed brightness value.
     * Defined by OperationRanges::getBrightnessMaxValue().
     */
    static constexpr float MAX_BRIGHTNESS_VALUE = OperationRanges::getBrightnessMaxValue();

    /**
     * @brief Default brightness value.
     * Defined by OperationRanges::getBrightnessDefaultValue().
     */
    static constexpr float DEFAULT_BRIGHTNESS_VALUE = OperationRanges::getBrightnessDefaultValue();

    /**
     * @brief Applies the brightness adjustment.
     *
     * This method provides sequential execution capability for the whites adjustment operation.
     * While primarily replaced by the fused pipeline system (appendToFusedPipeline), it remains
     * available for specific use cases such as debugging, testing, or standalone operation execution.
     *
     * Reads the "value" parameter from the descriptor and applies the brightness
     * formula to every color channel (RGB) of every pixel in the working image.
     * The alpha channel is left unchanged.
     * Performs a validation check to ensure the value is within the defined range [MIN_BRIGHTNESS_VALUE, MAX_BRIGHTNESS_VALUE].
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
     * This method implements the fusion logic specific to the Brightness adjustment,
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
     *               brightness adjustment value and other relevant parameters.
     * @return A new Halide::Func representing the output of this operation,
     *         which can be used as input for the next operation in the fused pipeline.
     *         The returned function encapsulates the logic to adjust the brightness
     *         operating directly on the pixel data stream using the shared coordinate variables.
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
