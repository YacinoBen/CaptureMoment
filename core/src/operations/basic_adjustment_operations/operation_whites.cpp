/**
 * @file operation_whites.cpp
 * @brief Implementation of OperationWhites
 * @author CaptureMoment Team
 * @date 2025
 */

#include "operations/basic_adjustment_operations/operation_whites.h"
#include "common/error_handling/core_error.h"

#include <spdlog/spdlog.h>
#include <algorithm>
#include <limits>

namespace CaptureMoment::Core::Operations {

// ============================================================================
// Internal Helper: Shared Halide Logic
// ============================================================================

template<typename InputType>
Halide::Func applyWhitesAdjustment(
    const InputType& input,
    const Halide::Param<float>& param_whites,
    const Halide::Var& x,
    const Halide::Var& y,
    const Halide::Var& c,
    float low_threshold = 0.7f,
    float high_threshold = 1.0f)
{
    Halide::Func whites_func("whites_op");
    Halide::Func luminance_func("luminance_whites");
    Halide::Func mask_func("mask_whites");

    // Calculate Luminance
    luminance_func(x, y) = 0.299f * input(x, y, 0) +
                           0.587f * input(x, y, 1) +
                           0.114f * input(x, y, 2);

    // Mask: 0.0 below 0.7, ramp to 1.0 at 1.0
    // Note: Typically Whites targets the very top (e.g. > 0.9),
    // adjusting low_threshold separates it from Highlights.
    mask_func(x, y) = Halide::select(
        luminance_func(x, y) <= low_threshold,
        0.0f,
        luminance_func(x, y) >= high_threshold,
        1.0f,
        (luminance_func(x, y) - low_threshold) / (high_threshold - low_threshold)
        );

    
    // Safety: Clamp the input parameter to the operation's defined valid range.
    Halide::Expr safe_val = Halide::clamp(
        param_whites, 
        OperationWhites::MIN_WHITES_VALUE, 
        OperationWhites::MAX_WHITES_VALUE
    );

    // Apply Adjustment
    whites_func(x, y, c) = Halide::select(
        c < 3,
        input(x, y, c) + safe_val * mask_func(x, y),
        input(x, y, c) // Alpha unchanged
        );

    return whites_func;
}

// ============================================================================
// IOperationFusionLogic Implementation
// ============================================================================

Halide::Func OperationWhites::appendToFusedPipeline(
    const Halide::Func& input_func,
    const Halide::Var& x,
    const Halide::Var& y,
    const Halide::Var& c,
    const Halide::Param<float>& param
    ) const
{
    // Halide's constant folding will optimize away additions by zero.
    spdlog::trace("OperationWhites::appendToFusedPipeline: Fusing with Halide Param (In-Graph Clamped)");
    return applyWhitesAdjustment(input_func, param, x, y, c);
}

} // namespace CaptureMoment::Core::Operations
