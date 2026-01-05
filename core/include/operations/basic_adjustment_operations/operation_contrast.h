/**
 * @file operation_contrast.h
 * @brief Concrete implementation of Contrast adjustment
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once
#include "operations/i_operation.h"

namespace CaptureMoment::Core {

namespace Operations {
/**
 * @class OperationContrast
 * @brief Adjusts the contrast of an image region.
 * * This operation modifies the contrast by scaling pixel values around the midpoint (0.5).
 * * Increasing contrast makes bright areas brighter and dark areas darker.
 * * Decreasing contrast makes the image appear flatter.
 * * **Algorithm**:
 * For each pixel `p` and channel `c` (excluding alpha):
 * \f$ p_c = 0.5 + (p_c - 0.5) \times (1.0 + \text{value}) \f$
 * * **Parameters**:
 * - `value` (float): The contrast adjustment factor.
 * - Range: Typically [-1.0, 1.0]
 * - 0.0: No change
 * - > 0: Increase contrast
 * - < 0: Decrease contrast
 */
class OperationContrast : public IOperation
{
public:
    // --- Metadata ---
    [[nodiscard]] OperationType type() const override { return OperationType::Contrast; }
    [[nodiscard]] const char* name() const override { return "Contrast"; }

    // --- Execution ---
    /**
     * @brief Applies the contrast adjustment.
     * * Reads the "value" parameter from the descriptor and applies the contrast
     * * formula to every color channel (RGB) of every pixel in the region.
     * * The alpha channel is left unchanged.
     * * @param input The region to modify.
     * @param params Must contain a "value" (float) parameter.
     * @return true if successful.
     */
    [[nodiscard]] bool execute(Common::ImageRegion& input, const OperationDescriptor& params) override;
};

} // namespace Operations

} // namespace CaptureMoment::Core
