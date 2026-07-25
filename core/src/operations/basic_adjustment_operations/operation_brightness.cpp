/**
 * @file operation_brightness.cpp
 * @brief Implementation of OperationBrightness
 * @author CaptureMoment Team
 * @date 2025
 */

#include "operations/basic_adjustment_operations/operation_brightness.h"

#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::Operations {

template<typename InputType>
Halide::Func applyBrightnessAdjustment(
    const InputType& input,
    const Halide::Param<float>& param_brightness,
    const Halide::Var& x,
    const Halide::Var& y,
    const Halide::Var& c)
{
    Halide::Func brightness_func("brightness_op");

    // Safety: Clamp the input parameter to the operation's defined valid range.
    // This ensures robustness even if the cache passes an unexpected value.
    Halide::Expr safe_brightness = Halide::clamp(
        param_brightness, 
        OperationBrightness::MIN_BRIGHTNESS_VALUE, 
        OperationBrightness::MAX_BRIGHTNESS_VALUE
    );

    // Additive brightness adjustment
    brightness_func(x, y, c) = Halide::select(
        c == 0,
        Halide::clamp(input(x, y, c) + safe_brightness, 0.0f, 1.0f),
        input(x, y, c) // Alpha unchanged
        );

    return brightness_func;
}

// ============================================================================
// IOperationFusionLogic Implementation
// ============================================================================

Halide::Func OperationBrightness::appendToFusedPipeline(
    const Halide::Func& input_func,
    const Halide::Var& x,
    const Halide::Var& y,
    const Halide::Var& c,
    const Halide::Param<float>& param
    ) const
{
    // to allow dynamic updates via the cache without recompilation.
    // Halide's optimizer (Constant Folding) will handle the math if the value is 0.
    spdlog::trace("OperationBrightness::appendToFusedPipeline: Fusing with Halide Param (In-Graph Clamped)");
    return applyBrightnessAdjustment(input_func, param, x, y, c);
}

} // namespace CaptureMoment::Core::Operations
