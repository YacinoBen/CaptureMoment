/**
 * @file operation_highlights.h
 * @brief Concrete implementation of Highlights adjustment
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once
#include "i_operation.h"

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
 * - Range: Typically [-1.0, 1.0]
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

    // --- Execution ---
    /**
     * @brief Applies the highlights adjustment.
     * * Reads the "value" parameter from the descriptor and applies the highlights
     * * formula to every color channel (RGB) of every pixel in the region,
     * * primarily affecting pixels with higher luminance.
     * * The alpha channel is left unchanged.
     * * @param input The region to modify.
     * @param params Must contain a "value" (float) parameter.
     * @return true if successful.
     */
    [[nodiscard]] bool execute(Common::ImageRegion& input, const OperationDescriptor& params) override;
};

} // namespace Operations

} // namespace CaptureMoment::Core
