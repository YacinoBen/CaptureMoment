/**
 * @file operation_contrast.cpp
 * @brief Implementation of OperationContrast
 * @author CaptureMoment Team
 * @date 2025
 */

#include "operations/basic_adjustment_operations/operation_contrast.h"

#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::Operations {

template<typename InputType>
Halide::Func applyContrastAdjustment(
    const InputType& input,
    const Halide::Param<float>& param_contrast,
    const Halide::Var& x,
    const Halide::Var& y,
    const Halide::Var& c)
{
    Halide::Func contrast_func("contrast_op");

    // Safety: Clamp the input parameter to the operation's defined valid range.
    Halide::Expr safe_contrast = clamp(
        param_contrast,
        OperationContrast::MIN_CONTRAST_VALUE,
        OperationContrast::MAX_CONTRAST_VALUE
        );

    // Multiplicative contrast centered at 0.5 (Mid-gray in Oklab L-channel)
    // Formula: 0.5 + (L - 0.5) * ContrastFactor
    //
    // IMPORTANT: We do NOT clamp the result of this operation here.
    // Clamping L inside Oklab while 'a' and 'b' are non-zero causes severe hue shifts
    // during the oklab_to_rgb conversion. Values > 1.0 or < 0.0 are safely handled
    // later when clamping the final RGB output.
    contrast_func(x, y, c) = select(
        c == 0,
        0.5f + (input(x, y, c) - 0.5f) * safe_contrast,
        input(x, y, c) // a, b, and Alpha channels remain untouched
        );

    return contrast_func;
}

// ============================================================================
// IOperationFusionLogic Implementation
// ============================================================================

Halide::Func OperationContrast::appendToFusedPipeline(
    const Halide::Func& input_func,
    const Halide::Var& x,
    const Halide::Var& y,
    const Halide::Var& c,
    const Halide::Param<float>& param
    ) const
{
    // Halide's optimizer will simplify the math if the contrast factor is 1.0.
    spdlog::trace("OperationContrast::appendToFusedPipeline: Fusing with Halide Param (In-Graph Clamped)");
    return applyContrastAdjustment(input_func, param, x, y, c);
}

} // namespace CaptureMoment::Core::Operations
