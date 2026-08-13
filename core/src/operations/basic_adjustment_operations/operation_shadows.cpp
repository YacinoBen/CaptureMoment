/**
 * @file operation_shadows.cpp
 * @brief Implementation of OperationShadows
 * @author CaptureMoment Team
 * @date 2025
 */

#include "operations/basic_adjustment_operations/operation_shadows.h"

#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::Operations {

template<typename InputType>
Halide::Func applyShadowsAdjustment(
    const InputType& input,
    const Halide::Param<float>& param_shadows,
    const Halide::Var& x,
    const Halide::Var& y,
    const Halide::Var& c)
{
    Halide::Func shadows_func("shadows_op");

    // 1. In Oklab, channel 0 IS the perceptual luminance.
    Halide::Expr L = input(x, y, 0);

    // 2. Create a smooth mask targeting the shadow/lower-midtone range.
    // Shadows typically cover a wider range than "Blacks" (e.g., 0.0 to 0.5).
    // Adjust the 0.5f threshold if you want a tighter or wider shadow roll-off.
    constexpr float shadow_cutoff = 0.5f;

    Halide::Expr t = clamp((shadow_cutoff - L) / shadow_cutoff, 0.0f, 1.0f);

    // Apply the smoothstep polynomial curve: 3t^2 - 2t^3
    Halide::Expr mask = t * t * (3.0f - 2.0f * t);

    // 3. Clamp the input parameter to the operation's defined valid range.
    Halide::Expr safe_val = clamp(
        param_shadows,
        OperationShadows::MIN_SHADOWS_VALUE,
        OperationShadows::MAX_SHADOWS_VALUE
        );

    // 4. Apply adjustment only to channel 0 (L)
    // NO CLAMP here to prevent hue shifts during oklab_to_rgb conversion.
    shadows_func(x, y, c) = select(
        c == 0,
        L + safe_val * mask,
        input(x, y, c) // a, b, and Alpha channels remain untouched
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
