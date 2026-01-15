/**
 * @file operation_blacks.h
 * @brief Concrete implementation of Blacks adjustment
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once
#include "operations/i_operation.h"
#include "operations/operation_ranges.h"

namespace CaptureMoment::Core {

namespace Operations {
/**
 * @class OperationBlacks
 * @brief Adjusts the black point of an image region.
 * * This operation modifies the luminance of the darkest areas of the image,
 * * effectively shifting the black point.
 * * Increasing blacks brightens the overall image and makes blacks less dark.
 * * Decreasing blacks darkens the overall image and makes blacks darker.
 * * **Algorithm** (Approximation - actual implementations can be more complex):
 * For each pixel `p` and channel `c` (excluding alpha), if the luminance is below a low threshold:
 * \f$ p_c = p_c + \text{value} \times \text{adjustment_factor} \f$
 * This is a simplified version focusing on the lower part of the luminance range.
 * * **Parameters**:
 * - `value` (float): The blacks adjustment factor.
 * - Range: Defined by OperationRanges::getBlacksMinValue() and OperationRanges::getBlacksMaxValue()
 * - Default: OperationRanges::getBlacksDefaultValue() (typically 0.0f, No change)
 * - > 0: Brighten blacks (make them less black)
 * - < 0: Darken blacks (make them more black)
 */
class OperationBlacks : public IOperation
{
public:
    // --- Metadata ---
    [[nodiscard]] OperationType type() const override { return OperationType::Blacks; }
    [[nodiscard]] const char* name() const override { return "Blacks"; }

    // --- Range Access (via the centralized ranges) ---
    /**
     * @brief Minimum allowed blacks value.
     * Defined by OperationRanges::getBlacksMinValue().
     */
    static constexpr float MIN_BLACKS_VALUE = OperationRanges::getBlacksMinValue();

    /**
     * @brief Maximum allowed blacks value.
     * Defined by OperationRanges::getBlacksMaxValue().
     */
    static constexpr float MAX_BLACKS_VALUE = OperationRanges::getBlacksMaxValue();

    /**
     * @brief Default blacks value.
     * Defined by OperationRanges::getBlacksDefaultValue().
     */
    static constexpr float DEFAULT_BLACKS_VALUE = OperationRanges::getBlacksDefaultValue();

    // --- Execution ---
    /**
     * @brief Applies the blacks adjustment.
     * * Reads the "value" parameter from the descriptor and applies the blacks
     * * formula to every color channel (RGB) of every pixel in the region,
     * * primarily affecting pixels with very low luminance (the "blacks").
     * * The alpha channel is left unchanged.
     * * Performs a validation check to ensure the value is within the defined range [MIN_BLACKS_VALUE, MAX_BLACKS_VALUE].
     * * @param input The region to modify.
     * @param params Must contain a "value" (float) parameter.
     * @return true if successful.
     */
    [[nodiscard]] bool execute(Common::ImageRegion& input, const OperationDescriptor& params) override;
};

} // namespace Operations

} // namespace CaptureMoment::Core
