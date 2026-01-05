/**
 * @file operation_shadows.h
 * @brief Concrete implementation of Shadows adjustment
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once
#include "i_operation.h"

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
 * - Range: Typically [-1.0, 1.0]
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

    // --- Execution ---
    /**
     * @brief Applies the shadows adjustment.
     * * Reads the "value" parameter from the descriptor and applies the shadows
     * * formula to every color channel (RGB) of every pixel in the region,
     * * primarily affecting pixels with lower luminance.
     * * The alpha channel is left unchanged.
     * * @param input The region to modify.
     * @param params Must contain a "value" (float) parameter.
     * @return true if successful.
     */
    [[nodiscard]] bool execute(Common::ImageRegion& input, const OperationDescriptor& params) override;
};

} // namespace Operations

} // namespace CaptureMoment::Core
