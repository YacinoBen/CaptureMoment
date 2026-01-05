/**
 * @file operation_brightness.h
 * @brief Concrete implementation of Brightness adjustment
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once
#include "operations/i_operation.h"

namespace CaptureMoment::Core {

namespace Operations {
/**
 * @class BrightnessOperation
 * @brief Adjusts the brightness of an image region.
 * * This operation performs a simple additive adjustment to the pixel values.
 * * **Algorithm**:
 * For each pixel `p` and channel `c`:
 * \f$ p_c = p_c + \text{value} \f$
 * * **Parameters**:
 * - `value` (float): The brightness offset.
 * - Range: Typically [-1.0, 1.0]
 * - 0.0: No change
 * - > 0: Brighter
 * - < 0: Darker
 */
class OperationBrightness : public IOperation
{
public:
    // --- Metadata ---
    [[nodiscard]] OperationType type() const override { return OperationType::Brightness; }
    [[nodiscard]] const char* name() const override { return "Brightness"; }

    // --- Execution ---
    /**
     * @brief Applies the brightness adjustment.
     * * Reads the "value" parameter from the descriptor and adds it to every
     * channel of every pixel in the region.
     * * @param input The region to modify.
     * @param params Must contain a "value" (float) parameter.
     * @return true if successful.
     */
    [[nodiscard]] bool execute(Common::ImageRegion& input, const OperationDescriptor& params) override;
};

} // namespace Operations

} // namespace CaptureMoment::Core
