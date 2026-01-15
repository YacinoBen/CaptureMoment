/**
 * @file operation_shadows.h
 * @brief Concrete implementation of Shadows adjustment
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once
#include "operations/i_operation.h"
#include "operations/operation_ranges.h" // Include the new ranges header

namespace CaptureMoment::Core {

namespace Operations {
/**
 * @class OperationShadows
 * @brief Adjusts the shadows (dark tones) of an image region.
 * * This operation modifies the luminance of the darker areas of the image.
 * * Increasing shadows brightens the dark areas.
 * * Decreasing shadows darkens the dark areas.
 * * **Algorithm** (Approximation - actual implementations can be more complex):
 * For each pixel `p` and channel `c` (excluding alpha), if the luminance is below a threshold:
 * \f$ p_c = p_c + \text{value} \times \text{adjustment_factor_based_on_luminance} \f$
 * * **Parameters**:
 * - `value` (float): The shadows adjustment factor.
 * - Range: Defined by OperationRanges::getShadowsMinValue() and OperationRanges::getShadowsMaxValue()
 * - Default: OperationRanges::getShadowsDefaultValue() (typically 0.0f, No change)
 * - 0.0: No change
 * - > 0: Brighten shadows
 * - < 0: Darken shadows
 */
class OperationShadows : public IOperation
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

    // --- Execution ---
    /**
     * @brief Applies the shadows adjustment.
     * * Reads the "value" parameter from the descriptor and applies the shadows
     * * formula to every color channel (RGB) of every pixel in the region,
     * * primarily affecting pixels with lower luminance.
     * * The alpha channel is left unchanged.
     * * Performs a validation check to ensure the value is within the defined range [MIN_SHADOWS_VALUE, MAX_SHADOWS_VALUE].
     * * @param input The region to modify.
     * @param params Must contain a "value" (float) parameter.
     * @return true if successful.
     */
    [[nodiscard]] bool execute(Common::ImageRegion& input, const OperationDescriptor& params) override;
};

} // namespace Operations

} // namespace CaptureMoment::Core
