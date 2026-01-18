/**
 * @file operation_whites.h
 * @brief Concrete implementation of Whites adjustment
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once
#include "operations/i_operation.h"
#include "operations/operation_ranges.h" // Include the new ranges header

namespace CaptureMoment::Core {

namespace Operations {
/**
 * @class OperationWhites
 * @brief Adjusts the white point of an image region.
 * * This operation modifies the luminance of the brightest areas of the image,
 * * effectively shifting the white point.
 * * Increasing whites brightens the overall image and makes whites less bright (more gray).
 * * Decreasing whites darkens the overall image and makes whites brighter.
 * * **Algorithm** (Approximation - actual implementations can be more complex):
 * For each pixel `p` and channel `c` (excluding alpha), if the luminance is above a high threshold:
 * \f$ p_c = p_c + \text{value} \times \text{adjustment_factor} \f$
 * This is a simplified version focusing on the upper part of the luminance range.
 * * **Parameters**:
 * - `value` (float): The whites adjustment factor.
 * - Range: Defined by OperationRanges::getWhitesMinValue() and OperationRanges::getWhitesMaxValue()
 * - Default: OperationRanges::getWhitesDefaultValue() (typically 0.0f, No change)
 * - > 0: Darken whites (make them less bright)
 * - < 0: Brighten whites (make them more bright)
 */
class OperationWhites : public IOperation
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
     * Reads the "value" parameter from the descriptor and applies the whites
     * formula to every color channel (RGB) of every pixel in the working image,
     * primarily affecting pixels with very high luminance (the "whites").
     * The alpha channel is left unchanged.
     * Performs a validation check to ensure the value is within the defined range [MIN_WHITES_VALUE, MAX_WHITES_VALUE].
     * @param working_image The hardware-agnostic image buffer to modify.
     * @param params Must contain a "value" (float) parameter.
     * @return true if successful.
     */
    [[nodiscard]] bool execute(ImageProcessing::IWorkingImageHardware& working_image, const OperationDescriptor& params) override;
};

} // namespace Operations

} // namespace CaptureMoment::Core
