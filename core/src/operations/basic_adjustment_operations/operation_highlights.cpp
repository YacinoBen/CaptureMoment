/**
 * @file operation_highlights.cpp
 * @brief Implementation of OperationHighlights
 * @author CaptureMoment Team
 * @date 2025
 */

#include "operations/basic_adjustment_operations/operation_highlights.h"

#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::Operations {

template<typename InputType>
Halide::Func applyHighlightsAdjustment(
    const InputType& input,
    const Halide::Param<float>& param_highlights,
    const Halide::Var& x,
    const Halide::Var& y,
    const Halide::Var& c)
{
    Halide::Func highlights_func("highlights_op");

    // 1. In Oklab, channel 0 IS the perceptual luminance. No RGB luminance formula needed.
    Halide::Expr L = input(x, y, 0);

    // 2. Create a smooth mask (Smoothstep equivalent)
    // Normalize L to a 0.0 - 1.0 range based on the 0.7 to 1.0 thresholds
    Halide::Expr t = clamp((L - 0.7f) / (1.0f - 0.7f), 0.0f, 1.0f);

    // Apply the smoothstep polynomial curve: 3t^2 - 2t^3
    // This prevents hard edges/banding in the highlights roll-off.
    Halide::Expr mask = t * t * (3.0f - 2.0f * t);

    // 3. Clamp the input parameter to the operation's defined valid range.
    Halide::Expr safe_val = clamp(
        param_highlights,
        OperationHighlights::MIN_HIGHLIGHTS_VALUE,
        OperationHighlights::MAX_HIGHLIGHTS_VALUE
        );

    // 4. Apply adjustment only to channel 0 (L)
    highlights_func(x, y, c) = select(
        c == 0,
        L + safe_val * mask,    // Add value weighted by the smooth mask
        input(x, y, c)          // a, b, and Alpha channels remain untouched
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
