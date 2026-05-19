/**
 * @file operation_blacks.cpp
 * @brief Implementation of OperationBlacks
 * @author CaptureMoment Team
 * @date 2025
 */

#include "operations/basic_adjustment_operations/operation_blacks.h"
#include "common/error_handling/core_error.h"

#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>
#include <limits>

namespace CaptureMoment::Core::Operations {

// ============================================================================
// Internal Helper: Shared Halide Logic
// ============================================================================

template<typename InputType>
Halide::Func applyBlacksAdjustment(
    const InputType& input,
    const Halide::Param<float>& param_blacks,
    const Halide::Var& x,
    const Halide::Var& y,
    const Halide::Var& c,
    float low_threshold = 0.0f,
    float high_threshold = 0.3f)
{
    Halide::Func blacks_func("blacks_op");
    Halide::Func luminance_func("luminance_blacks");
    Halide::Func mask_func("mask_blacks");

    // Calculate Luminance (Rec. 709)
    luminance_func(x, y) = 0.299f * input(x, y, 0) +
                           0.587f * input(x, y, 1) +
                           0.114f * input(x, y, 2);

    // Calculate Mask: 1.0 in deep blacks, fading to 0.0 at high_threshold
    mask_func(x, y) = Halide::select(
        luminance_func(x, y) >= high_threshold,
        0.0f,
        luminance_func(x, y) <= low_threshold,
        1.0f,
        (high_threshold - luminance_func(x, y)) / (high_threshold - low_threshold)
        );


    Halide::Expr safe_blacks_val = Halide::clamp(
        param_blacks, 
        OperationBlacks::MIN_BLACKS_VALUE, 
        OperationBlacks::MAX_BLACKS_VALUE
    );

    // Apply Adjustment
    blacks_func(x, y, c) = Halide::select(
        c < 3,
        input(x, y, c) + safe_blacks_val * mask_func(x, y),
        input(x, y, c) // Alpha unchanged
        );

    return blacks_func;
}

// ============================================================================
// IOperationFusionLogic Implementation
// ============================================================================

Halide::Func OperationBlacks::appendToFusedPipeline(
    const Halide::Func& input_func,
    const Halide::Var& x,
    const Halide::Var& y,
    const Halide::Var& c,
    const Halide::Param<float>& param
    ) const
{
    spdlog::trace("OperationBlacks::appendToFusedPipeline: Fusing with Halide Param");
    return applyBlacksAdjustment(input_func, param, x, y, c);
}

} // namespace CaptureMoment::Core::Operations
