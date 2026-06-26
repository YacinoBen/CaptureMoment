/**
 * @file operation_contrast.cpp
 * @brief Implementation of OperationContrast
 * @author CaptureMoment Team
 * @date 2025
 */

#include "operations/basic_adjustment_operations/operation_contrast.h"
#include "common/error_handling/core_error.h"

#include <spdlog/spdlog.h>
#include <algorithm>
#include <limits>

namespace CaptureMoment::Core::Operations {

// ============================================================================
// Internal Helper: Shared Halide Logic
// ============================================================================

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
    Halide::Expr safe_contrast = Halide::clamp(
        param_contrast, 
        OperationContrast::MIN_CONTRAST_VALUE, 
        OperationContrast::MAX_CONTRAST_VALUE
    );

    // Multiplicative contrast centered at 0.5 (Mid-gray)
    // Formula: 0.5 + (Input - 0.5) * ContrastFactor
    // Result is clamped to [0.0, 1.0] to maintain valid color space.
    contrast_func(x, y, c) = Halide::select(
        c < 3,
        Halide::clamp(0.5f + (input(x, y, c) - 0.5f) * safe_contrast, 0.0f, 1.0f),
        input(x, y, c) // Alpha unchanged
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
