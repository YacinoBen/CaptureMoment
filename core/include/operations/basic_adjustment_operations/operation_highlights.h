/**
 * @file operation_highlights.h
 * @brief Concrete implementation of Highlights adjustment
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once
#include "operations/i_operation.h"
#include "operations/operation_ranges.h"

namespace CaptureMoment::Core {

namespace Operations {
/**
 * @class OperationHighlights
 * @brief Adjusts the highlights (bright tones) of an image region.
 * * This operation modifies the luminance of the brighter areas of the image.
 * * Increasing highlights brightens the bright areas.
 * * Decreasing highlights darkens the bright areas.
 * * **Algorithm** (Approximation - actual implementations can be more complex):
 * For each pixel `p` and channel `c` (excluding alpha), if the luminance is above a threshold:
 * \f$ p_c = p_c + \text{value} \times \text{adjustment_factor_based_on_luminance} \f$
 * * **Parameters**:
 * - `value` (float): The highlights adjustment factor.
 * - Range: Defined by OperationRanges::getHighlightsMinValue() and OperationRanges::getHighlightsMaxValue()
 * - Default: OperationRanges::getHighlightsDefaultValue() (typically 0.0f, No change)
 * - 0.0: No change
 * - > 0: Brighten highlights
 * - < 0: Darken highlights
 */
class OperationHighlights : public IOperation
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

    // --- Execution ---
    /**
     * @brief Applies the highlights adjustment.
     * * Reads the "value" parameter from the descriptor and applies the highlights
     * * formula to every color channel (RGB) of every pixel in the region,
     * * primarily affecting pixels with higher luminance.
     * * The alpha channel is left unchanged.
     * * Performs a validation check to ensure the value is within the defined range [MIN_HIGHLIGHTS_VALUE, MAX_HIGHLIGHTS_VALUE].
     * * @param input The region to modify.
     * @param params Must contain a "value" (float) parameter.
     * @return true if successful.
     */
    [[nodiscard]] bool execute(Common::ImageRegion& input, const OperationDescriptor& params) override;
};

} // namespace Operations

} // namespace CaptureMoment::Core
