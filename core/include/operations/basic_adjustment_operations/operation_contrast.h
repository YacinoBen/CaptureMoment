/**
 * @file operation_contrast.h
 * @brief Concrete implementation of Contrast adjustment
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once
#include "operations/i_operation.h"
#include "operations/operation_ranges.h"

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
 * - Range: Defined by OperationRanges::getContrastMinValue() and OperationRanges::getContrastMaxValue()
 * - Default: OperationRanges::getContrastDefaultValue() (typically 1.0f, No change)
 * - 1.0: No change
 * - > 1.0: Increase contrast
 * - < 1.0: Decrease contrast
 */
class OperationContrast : public IOperation
{
public:
    // --- Metadata ---
    [[nodiscard]] OperationType type() const override { return OperationType::Contrast; }
    [[nodiscard]] const char* name() const override { return "Contrast"; }

    // --- Range Access (via the centralized ranges) ---
    /**
     * @brief Minimum allowed contrast value.
     * Defined by OperationRanges::getContrastMinValue().
     */
    static constexpr float MIN_CONTRAST_VALUE = OperationRanges::getContrastMinValue();

    /**
     * @brief Maximum allowed contrast value.
     * Defined by OperationRanges::getContrastMaxValue().
     */
    static constexpr float MAX_CONTRAST_VALUE = OperationRanges::getContrastMaxValue();

    /**
     * @brief Default contrast value.
     * Defined by OperationRanges::getContrastDefaultValue().
     */
    static constexpr float DEFAULT_CONTRAST_VALUE = OperationRanges::getContrastDefaultValue();

    // --- Execution ---
    /**
     * @brief Applies the contrast adjustment.
     * * Reads the "value" parameter from the descriptor and applies the contrast
     * * formula to every color channel (RGB) of every pixel in the region.
     * * The alpha channel is left unchanged.
     * * Performs a validation check to ensure the value is within the defined range [MIN_CONTRAST_VALUE, MAX_CONTRAST_VALUE].
     * * @param input The region to modify.
     * @param params Must contain a "value" (float) parameter.
     * @return true if successful.
     */
    [[nodiscard]] bool execute(Common::ImageRegion& input, const OperationDescriptor& params) override;
};

} // namespace Operations

} // namespace CaptureMoment::Core
