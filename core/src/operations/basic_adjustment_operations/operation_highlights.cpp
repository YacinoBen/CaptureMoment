/**
 * @file operation_highlights.cpp
 * @brief Implementation of OperationHighlights
 * @author CaptureMoment Team
 * @date 2025
 */

#include "operations/basic_adjustment_operations/operation_highlights.h"
#include "common/error_handling/core_error.h"

#include <spdlog/spdlog.h>
#include <algorithm>
#include <limits>

namespace CaptureMoment::Core::Operations {

// ============================================================================
// Internal Helper: Shared Halide Logic
// ============================================================================

template<typename InputType>
Halide::Func applyHighlightsAdjustment(
    const InputType& input,
    const Halide::Param<float>& param_highlights,
    const Halide::Var& x,
    const Halide::Var& y,
    const Halide::Var& c,
    float low_threshold = 0.7f,
    float high_threshold = 1.0f)
{
    Halide::Func highlights_func("highlights_op");
    Halide::Func luminance_func("luminance_highlights");
    Halide::Func mask_func("mask_highlights");

    // Calculate Luminance
    luminance_func(x, y) = 0.299f * input(x, y, 0) +
                           0.587f * input(x, y, 1) +
                           0.114f * input(x, y, 2);

    // Mask: 0.0 below 0.7, ramp to 1.0 at 1.0
    mask_func(x, y) = Halide::select(
        luminance_func(x, y) <= low_threshold,
        0.0f,
        luminance_func(x, y) >= high_threshold,
        1.0f,
        (luminance_func(x, y) - low_threshold) / (high_threshold - low_threshold)
        );

    // Safety: Clamp the input parameter to the operation's defined valid range.
    Halide::Expr safe_val = Halide::clamp(
        param_highlights, 
        OperationHighlights::MIN_HIGHLIGHTS_VALUE, 
        OperationHighlights::MAX_HIGHLIGHTS_VALUE
    );

    // Apply Adjustment
    highlights_func(x, y, c) = Halide::select(
        c < 3,
        input(x, y, c) + safe_val * mask_func(x, y),
        input(x, y, c) // Alpha unchanged
        );

    return highlights_func;
}

// ============================================================================
// IOperationFusionLogic Implementation
// ============================================================================

Halide::Func OperationHighlights::appendToFusedPipeline(
    const Halide::Func& input_func,
    const Halide::Var& x,
    const Halide::Var& y,
    const Halide::Var& c,
    const Halide::Param<float>& param
    ) const
{
const Halide::Param<float>& param_highlights = param;

    // Halide's optimizer will simplify the math if the highlights adjustment is effectively zero.
    spdlog::trace("OperationHighlights::appendToFusedPipeline: Fusing with Halide Param (In-Graph Clamped)");
    return applyHighlightsAdjustment(input_func, param_highlights, x, y, c);
}

} // namespace CaptureMoment::Core::Operations
