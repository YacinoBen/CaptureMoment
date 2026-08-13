/**
 * @file operation_whites.cpp
 * @brief Implementation of OperationWhites
 * @author CaptureMoment Team
 * @date 2025
 */

#include "operations/basic_adjustment_operations/operation_whites.h"

#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::Operations {

template<typename InputType>
Halide::Func applyWhitesAdjustment(
    const InputType& input,
    const Halide::Param<float>& param_whites,
    const Halide::Var& x,
    const Halide::Var& y,
    const Halide::Var& c)
{
    Halide::Func whites_func("whites_op");

    // 1. In Oklab, channel 0 IS the perceptual luminance.
    Halide::Expr L = input(x, y, 0);

    // 2. Create a smooth mask targeting ONLY the extreme whites.
    // Unlike Highlights (0.7 - 1.0), Whites should target the very top end (e.g., 0.85 - 1.0)
    // to adjust clipping without affecting the upper midtones.
    constexpr float whites_cutoff = 0.85f;

    Halide::Expr t = clamp((L - whites_cutoff) / (1.0f - whites_cutoff), 0.0f, 1.0f);

    // Apply the smoothstep polynomial curve: 3t^2 - 2t^3
    Halide::Expr mask = t * t * (3.0f - 2.0f * t);

    // 3. Clamp the input parameter to the operation's defined valid range.
    Halide::Expr safe_val = clamp(
        param_whites,
        OperationWhites::MIN_WHITES_VALUE,
        OperationWhites::MAX_WHITES_VALUE
        );

    // 4. Apply adjustment only to channel 0 (L)
    // NO CLAMP here to prevent hue shifts during oklab_to_rgb conversion.
    whites_func(x, y, c) = select(
        c == 0,
        L + safe_val * mask,
        input(x, y, c) // a, b, and Alpha channels remain untouched
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
