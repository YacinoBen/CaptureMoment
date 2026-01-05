/**
 * @file operation_whites.h
 * @brief Concrete implementation of Whites adjustment
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once
#include "i_operation.h"

namespace CaptureMoment::Core {

namespace Operations {
/**
 * @class OperationWhites
 * @brief Adjusts the white point of an image region.
 * * This operation modifies the luminance of the brightest areas of the image,
 * * effectively shifting the white point.
 * * Increasing whites brightens the overall image and makes whites brighter.
 * * Decreasing whites darkens the overall image and makes whites less bright.
 * * **Algorithm** (Approximation - actual implementations can be more complex):
 * For each pixel `p` and channel `c` (excluding alpha), if the luminance is above a high threshold:
 * \f$ p_c = p_c + \text{value} \times \text{adjustment_factor} \f$
 * This is a simplified version focusing on the upper part of the luminance range.
 * * **Parameters**:
 * - `value` (float): The whites adjustment factor.
 * - Range: Typically [-1.0, 1.0]
 * - 0.0: No change
 * - > 0: Brighten whites
 * - < 0: Darken whites
 */
class OperationWhites : public IOperation
{
public:
    // --- Metadata ---
    [[nodiscard]] OperationType type() const override { return OperationType::Whites; }
    [[nodiscard]] const char* name() const override { return "Whites"; }

    // --- Execution ---
    /**
     * @brief Applies the whites adjustment.
     * * Reads the "value" parameter from the descriptor and applies the whites
     * * formula to every color channel (RGB) of every pixel in the region,
     * * primarily affecting pixels with very high luminance (the "whites").
     * * The alpha channel is left unchanged.
     * * @param input The region to modify.
     * @param params Must contain a "value" (float) parameter.
     * @return true if successful.
     */
    [[nodiscard]] bool execute(Common::ImageRegion& input, const OperationDescriptor& params) override;
};

} // namespace Operations

} // namespace CaptureMoment::Core
