/**
 * @file operation_blacks.cpp
 * @brief Implementation of OperationBlacks
 * @author CaptureMoment Team
 * @date 2025
 */

#include "operations/basic_adjustment_operations/operation_blacks.h"

#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::Operations {

template<typename InputType>
Halide::Func applyBlacksAdjustment(
    const InputType& input,
    const Halide::Param<float>& param_blacks,
    const Halide::Var& x,
    const Halide::Var& y,
    const Halide::Var& c)
{
    Halide::Func blacks_func("blacks_op");

    // 1. In Oklab, channel 0 IS the perceptual luminance. No RGB formula needed.
    Halide::Expr L = input(x, y, 0);

    // 2. Create a smooth mask targeting the shadow range (0.0 to 0.3)
    // Note: We invert the subtraction compared to highlights because we want
    // full effect at L=0.0 and no effect at L=0.3.
    Halide::Expr t = clamp((0.3f - L) / (0.3f - 0.0f), 0.0f, 1.0f);

    // Apply the smoothstep polynomial curve: 3t^2 - 2t^3
    // This prevents harsh transitions in the midtones.
    Halide::Expr mask = t * t * (3.0f - 2.0f * t);

    // 3. Clamp the input parameter to the operation's defined valid range.
    Halide::Expr safe_val = clamp(
        param_blacks,
        OperationBlacks::MIN_BLACKS_VALUE,
        OperationBlacks::MAX_BLACKS_VALUE
        );

    // 4. Apply adjustment only to channel 0 (L)
    // NO CLAMP here to avoid hue shifts in the shadows during oklab_to_rgb conversion.
    blacks_func(x, y, c) = select(
        c == 0,
        L + safe_val * mask,
        input(x, y, c) // a, b, and Alpha channels remain untouched
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
