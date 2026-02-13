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
    float whites_value,
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

    // Apply Adjustment
    whites_func(x, y, c) = Halide::select(
        c < 3,
        input(x, y, c) + whites_value * mask_func(x, y),
        input(x, y, c) // Alpha unchanged
        );

    return whites_func;
}

// ============================================================================
// IOperation Implementation
// ============================================================================

std::expected<void, ErrorHandling::CoreError> OperationWhites::execute(
    ImageProcessing::IWorkingImageHardware& working_image,
    const OperationDescriptor& descriptor)
{
    // Step 1: Validation
    if (!working_image.isValid()) {
        spdlog::warn("OperationWhites::execute: Invalid working image provided");
        return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
    }

    if (!descriptor.enabled) {
        spdlog::trace("OperationWhites::execute: Operation is disabled, skipping");
        return {};
    }

    // Step 2: Extract Parameters
    auto value_res = descriptor.getParam<float>("value");
    if (!value_res) {
        spdlog::error("OperationWhites::execute: Failed to get 'value' parameter");
        return std::unexpected(ErrorHandling::CoreError::Unexpected);
    }
    float whites_value = value_res.value();

    // Step 3: No-Op Optimization
    if (std::abs(whites_value - OperationWhites::DEFAULT_WHITES_VALUE) < std::numeric_limits<float>::epsilon()) {
        spdlog::trace("OperationWhites::execute: Value is default, skipping");
        return {};
    }

    // Step 4: Clamp Value
    whites_value = std::clamp(whites_value, OperationWhites::MIN_WHITES_VALUE, OperationWhites::MAX_WHITES_VALUE);
    spdlog::debug("OperationWhites::execute: Applying whites with value={:.2f}", whites_value);

    // Step 5: Export & Execute
    auto cpu_copy_result = working_image.exportToCPUCopy();
    if (!cpu_copy_result) {
        spdlog::error("OperationWhites::execute: Failed to export working image to CPU");
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

        auto whites_func = applyWhitesAdjustment(input_buf, whites_value, x, y, c);
        whites_func.compute_root().parallel(y).vectorize(x, 8);
        whites_func.realize(input_buf);

        auto update_res = working_image.updateFromCPU(std::move(*cpu_region_ptr));
        if (!update_res) {
            spdlog::error("OperationWhites::execute: Failed to update working image from CPU");
            return std::unexpected(update_res.error());
        }

        return {};

    } catch (const std::exception& e) {
        spdlog::critical("OperationWhites::execute: Exception: {}", e.what());
        return std::unexpected(ErrorHandling::CoreError::Unexpected);
    }
}

// ============================================================================
// IOperationFusionLogic Implementation
// ============================================================================

Halide::Func OperationWhites::appendToFusedPipeline(
    const Halide::Func& input_func,
    const Halide::Var& x,
    const Halide::Var& y,
    const Halide::Var& c,
    const OperationDescriptor& params
    ) const
{
    auto value_res = params.getParam<float>("value");
    float whites_value = value_res.value_or(OperationWhites::DEFAULT_WHITES_VALUE);

    if (std::abs(whites_value - OperationWhites::DEFAULT_WHITES_VALUE) < std::numeric_limits<float>::epsilon()) {
        return input_func;
    }

    whites_value = std::clamp(whites_value, OperationWhites::MIN_WHITES_VALUE, OperationWhites::MAX_WHITES_VALUE);
    return applyWhitesAdjustment(input_func, whites_value, x, y, c);
}

// ============================================================================
// IOperationDefaultLogic Implementation
// ============================================================================

std::expected<void, ErrorHandling::CoreError> OperationWhites::executeOnImageRegion(
    Common::ImageRegion& region,
    const OperationDescriptor& params
    ) const
{
    if (!region.isValid()) {
        spdlog::error("[OperationWhites] executeOnImageRegion: Invalid ImageRegion.");
        return std::unexpected(ErrorHandling::CoreError::InvalidImageRegion);
    }

    auto value_res = params.getParam<float>("value");
    if (!value_res) {
        spdlog::warn("[OperationWhites] executeOnImageRegion: Param 'value' missing, skipping.");
        return {};
    }

    // TODO implement with OpenImageIO or OpenCV Or manually. To determine

    return {};
}

} // namespace CaptureMoment::Core::Operations
