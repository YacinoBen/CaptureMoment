/**
 * @file operation_blacks.h
 * @brief Concrete implementation of Blacks adjustment
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once
#include "operations/i_operation.h"

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
 * - Range: Typically [-1.0, 1.0]
 * - 0.0: No change
 * - > 0: Brighten blacks (make them less black)
 * - < 0: Darken blacks (make them more black)
 */
class OperationBlacks : public IOperation
{
public:
    // --- Metadata ---
    [[nodiscard]] OperationType type() const override { return OperationType::Blacks; }
    [[nodiscard]] const char* name() const override { return "Blacks"; }

    // --- Execution ---
    /**
     * @brief Applies the blacks adjustment.
     * * Reads the "value" parameter from the descriptor and applies the blacks
     * * formula to every color channel (RGB) of every pixel in the region,
     * * primarily affecting pixels with very low luminance (the "blacks").
     * * The alpha channel is left unchanged.
     * * @param input The region to modify.
     * @param params Must contain a "value" (float) parameter.
     * @return true if successful.
     */
    [[nodiscard]] bool execute(Common::ImageRegion& input, const OperationDescriptor& params) override;
};

} // namespace Operations

} // namespace CaptureMoment::Core
