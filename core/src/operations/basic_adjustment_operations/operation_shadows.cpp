/**
 * @file operation_shadows.cpp
 * @brief Implementation of OperationShadows
 * @author CaptureMoment Team
 * @date 2025
 */

#include "operations/basic_adjustment_operations/operation_shadows.h"
#include "common/error_handling/core_error.h"

#include <spdlog/spdlog.h>
#include <algorithm>
#include <limits>

namespace CaptureMoment::Core::Operations {

// ============================================================================
// Internal Helper: Shared Halide Logic
// ============================================================================

template<typename InputType>
Halide::Func applyShadowsAdjustment(
    const InputType& input,
    const Halide::Param<float>& param_shadows,
    const Halide::Var& x,
    const Halide::Var& y,
    const Halide::Var& c,
    float low_threshold = 0.0f,
    float high_threshold = 0.3f)
{
    Halide::Func shadows_func("shadows_op");
    Halide::Func luminance_func("luminance_shadows");
    Halide::Func mask_func("mask_shadows");

    // Calculate Luminance
    luminance_func(x, y) = 0.299f * input(x, y, 0) +
                           0.587f * input(x, y, 1) +
                           0.114f * input(x, y, 2);

    // Mask: 1.0 below 0.0, ramp to 0.0 at 0.3
    mask_func(x, y) = Halide::select(
        luminance_func(x, y) >= high_threshold,
        0.0f,
        luminance_func(x, y) <= low_threshold,
        1.0f,
        (high_threshold - luminance_func(x, y)) / (high_threshold - low_threshold)
        );

    // Safety: Clamp the input parameter to the operation's defined valid range.
    Halide::Expr safe_val = Halide::clamp(
        param_shadows, 
        OperationShadows::MIN_SHADOWS_VALUE, 
        OperationShadows::MAX_SHADOWS_VALUE
    );

    // Apply Adjustment
    shadows_func(x, y, c) = Halide::select(
        c < 3,
        input(x, y, c) + safe_val * mask_func(x, y),
        input(x, y, c) // Alpha unchanged
        );

    return shadows_func;
}

// ============================================================================
// IOperationFusionLogic Implementation
// ============================================================================

Halide::Func OperationShadows::appendToFusedPipeline(
    const Halide::Func& input_func,
    const Halide::Var& x,
    const Halide::Var& y,
    const Halide::Var& c,
    const Halide::Param<float>& param
    ) const
{
    // Zero adjustments are optimized out by Halide internally.
    spdlog::trace("OperationShadows::appendToFusedPipeline: Fusing with Halide Param (In-Graph Clamped)");
    return applyShadowsAdjustment(input_func, param, x, y, c);
}

} // namespace CaptureMoment::Core::Operations
