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
    float shadows_value,
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

    // Apply Adjustment
    shadows_func(x, y, c) = Halide::select(
        c < 3,
        input(x, y, c) + shadows_value * mask_func(x, y),
        input(x, y, c) // Alpha unchanged
        );

    return shadows_func;
}

// ============================================================================
// IOperation Implementation
// ============================================================================

std::expected<void, ErrorHandling::CoreError> OperationShadows::execute(
    ImageProcessing::IWorkingImageHardware& working_image,
    const OperationDescriptor& descriptor)
{
    // Step 1: Validation
    if (!working_image.isValid()) {
        spdlog::warn("OperationShadows::execute: Invalid working image provided");
        return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
    }

    if (!descriptor.enabled) {
        spdlog::trace("OperationShadows::execute: Operation is disabled, skipping");
        return {};
    }

    // Step 2: Extract Parameters
    auto value_res = descriptor.getParam<float>("value");
    if (!value_res) {
        spdlog::error("OperationShadows::execute: Failed to get 'value' parameter");
        return std::unexpected(ErrorHandling::CoreError::Unexpected);
    }
    float shadows_value = value_res.value();

    // Step 3: No-Op Optimization
    if (std::abs(shadows_value - OperationShadows::DEFAULT_SHADOWS_VALUE) < std::numeric_limits<float>::epsilon()) {
        spdlog::trace("OperationShadows::execute: Value is default, skipping");
        return {};
    }

    // Step 4: Clamp Value
    shadows_value = std::clamp(shadows_value, OperationShadows::MIN_SHADOWS_VALUE, OperationShadows::MAX_SHADOWS_VALUE);
    spdlog::debug("OperationShadows::execute: Applying shadows with value={:.2f}", shadows_value);

    // Step 5: Export & Execute
    auto cpu_copy_result = working_image.exportToCPUCopy();
    if (!cpu_copy_result) {
        spdlog::error("OperationShadows::execute: Failed to export working image to CPU");
        return std::unexpected(cpu_copy_result.error());
    }
    auto& cpu_region_ptr = cpu_copy_result.value();

    try {
        Halide::Var x, y, c;
        std::span<float> data_span = cpu_region_ptr->getBuffer();

        Halide::Buffer<float> input_buf(
            data_span.data(),
            static_cast<int>(cpu_region_ptr->m_width),
            static_cast<int>(cpu_region_ptr->m_height),
            static_cast<int>(cpu_region_ptr->m_channels)
            );

        auto shadows_func = applyShadowsAdjustment(input_buf, shadows_value, x, y, c);
        shadows_func.compute_root().parallel(y).vectorize(x, 8);
        shadows_func.realize(input_buf);

        auto update_res = working_image.updateFromCPU(std::move(*cpu_region_ptr));
        if (!update_res) {
            spdlog::error("OperationShadows::execute: Failed to update working image from CPU");
            return std::unexpected(update_res.error());
        }

        return {};

    } catch (const std::exception& e) {
        spdlog::critical("OperationShadows::execute: Exception: {}", e.what());
        return std::unexpected(ErrorHandling::CoreError::Unexpected);
    }
}

// ============================================================================
// IOperationFusionLogic Implementation
// ============================================================================

Halide::Func OperationShadows::appendToFusedPipeline(
    const Halide::Func& input_func,
    const Halide::Var& x,
    const Halide::Var& y,
    const Halide::Var& c,
    const OperationDescriptor& params
    ) const
{
    auto value_res = params.getParam<float>("value");
    float shadows_value = value_res.value_or(OperationShadows::DEFAULT_SHADOWS_VALUE);

    if (std::abs(shadows_value - OperationShadows::DEFAULT_SHADOWS_VALUE) < std::numeric_limits<float>::epsilon()) {
        return input_func;
    }

    shadows_value = std::clamp(shadows_value, OperationShadows::MIN_SHADOWS_VALUE, OperationShadows::MAX_SHADOWS_VALUE);
    return applyShadowsAdjustment(input_func, shadows_value, x, y, c);
}

// ============================================================================
// IOperationDefaultLogic Implementation
// ============================================================================

std::expected<void, ErrorHandling::CoreError> OperationShadows::executeOnImageRegion(
    Common::ImageRegion& region,
    const OperationDescriptor& params
    ) const
{
    if (!region.isValid()) {
        spdlog::error("[OperationShadows] executeOnImageRegion: Invalid ImageRegion.");
        return std::unexpected(ErrorHandling::CoreError::InvalidImageRegion);
    }

    auto value_res = params.getParam<float>("value");
    if (!value_res) {
        spdlog::warn("[OperationShadows] executeOnImageRegion: Param 'value' missing, skipping.");
        return {};
    }

    // TODO implement with OpenImageIO or OpenCV Or manually. To determine

    return {};
}

} // namespace CaptureMoment::Core::Operations
