/**
 * @file operation_brightness.h
 * @brief Concrete implementation of Brightness adjustment
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once
#include "operations/i_operation.h"
#include "operations/operation_ranges.h" // Include the new ranges header

namespace CaptureMoment::Core {

namespace Operations {
/**
 * @class OperationBrightness
 * @brief Adjusts the brightness of an image region.
 * * This operation performs a simple additive adjustment to the pixel values.
 * * **Algorithm**:
 * For each pixel `p` and channel `c`:
 * \f$ p_c = p_c + \text{value} \f$
 * * **Parameters**:
 * - `value` (float): The brightness offset.
 * - Range: Defined by OperationRanges::getBrightnessMinValue() and OperationRanges::getBrightnessMaxValue()
 * - Default: OperationRanges::getBrightnessDefaultValue() (typically 0.0f, No change)
 * - > 0: Brighter
 * - < 0: Darker
 */
class OperationBrightness : public IOperation
{
public:
    // --- Metadata ---
    [[nodiscard]] OperationType type() const override { return OperationType::Brightness; }
    [[nodiscard]] const char* name() const override { return "Brightness"; }

    // --- Range Access (via the centralized ranges) ---
    /**
     * @brief Minimum allowed brightness value.
     * Defined by OperationRanges::getBrightnessMinValue().
     */
    static constexpr float MIN_BRIGHTNESS_VALUE { OperationRanges::getBrightnessMinValue() };

    /**
     * @brief Maximum allowed brightness value.
     * Defined by OperationRanges::getBrightnessMaxValue().
     */
    static constexpr float MAX_BRIGHTNESS_VALUE { OperationRanges::getBrightnessMaxValue() };

    /**
     * @brief Default brightness value.
     * Defined by OperationRanges::getBrightnessDefaultValue().
     */
    static constexpr float DEFAULT_BRIGHTNESS_VALUE { OperationRanges::getBrightnessDefaultValue() };

    // --- Execution ---
    /**
     * @brief Applies the brightness adjustment.
     * * Reads the "value" parameter from the descriptor and adds it to every
     * channel of every pixel in the region.
     * * Performs a validation check to ensure the value is within the defined range [MIN_BRIGHTNESS_VALUE, MAX_BRIGHTNESS_VALUE].
     * * @param input The region to modify.
     * * @param params Must contain a "value" (float) parameter.
     * * @return true if successful.
     */
    [[nodiscard]] bool execute(Common::ImageRegion& input, const OperationDescriptor& params) override;
};

} // namespace Operations

} // namespace CaptureMoment::Core
