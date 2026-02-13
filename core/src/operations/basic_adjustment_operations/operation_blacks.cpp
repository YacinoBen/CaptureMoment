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
    float blacks_value,
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

    // Apply Adjustment
    blacks_func(x, y, c) = Halide::select(
        c < 3,
        input(x, y, c) + blacks_value * mask_func(x, y),
        input(x, y, c) // Alpha unchanged
        );

    return blacks_func;
}

// ============================================================================
// IOperation Implementation
// ============================================================================

std::expected<void, ErrorHandling::CoreError> OperationBlacks::execute(
    ImageProcessing::IWorkingImageHardware& working_image,
    const OperationDescriptor& descriptor)
{
    // Step 1: Validation
    if (!working_image.isValid()) {
        spdlog::warn("OperationBlacks::execute: Invalid working image provided");
        return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
    }

    if (!descriptor.enabled) {
        spdlog::trace("OperationBlacks::execute: Operation is disabled, skipping");
        return {};
    }

    // Step 2: Extract Parameters
    auto value_res = descriptor.getParam<float>("value");
    if (!value_res) {
        spdlog::error("OperationBlacks::execute: Failed to get 'value' parameter");
        return std::unexpected(ErrorHandling::CoreError::Unexpected);
    }
    float blacks_value = value_res.value();

    // Step 3: No-Op Optimization
    if (std::abs(blacks_value - OperationBlacks::DEFAULT_BLACKS_VALUE) < std::numeric_limits<float>::epsilon()) {
        spdlog::trace("OperationBlacks::execute: Value is default, skipping");
        return {};
    }

    // Step 4: Clamp Value
    blacks_value = std::clamp(blacks_value, OperationBlacks::MIN_BLACKS_VALUE, OperationBlacks::MAX_BLACKS_VALUE);
    spdlog::debug("OperationBlacks::execute: Applying blacks adjustment with value={:.2f}", blacks_value);

    // Step 5: Export & Execute
    auto cpu_copy_result = working_image.exportToCPUCopy();
    if (!cpu_copy_result) {
        spdlog::error("OperationBlacks::execute: Failed to export working image to CPU");
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

        auto blacks_func = applyBlacksAdjustment(input_buf, blacks_value, x, y, c);
        blacks_func.compute_root().parallel(y).vectorize(x, 8);
        blacks_func.realize(input_buf);

        // Update Working Image
        auto update_res = working_image.updateFromCPU(std::move(*cpu_region_ptr));
        if (!update_res) {
            spdlog::error("OperationBlacks::execute: Failed to update working image from CPU");
            return std::unexpected(update_res.error());
        }

        return {};

    } catch (const Halide::CompileError& e) {
        spdlog::critical("OperationBlacks::execute: Halide Compile Error: {}", e.what());
        return std::unexpected(ErrorHandling::CoreError::Unexpected);
    } catch (const Halide::RuntimeError& e) {
        spdlog::critical("OperationBlacks::execute: Halide Runtime Error: {}", e.what());
        return std::unexpected(ErrorHandling::CoreError::Unexpected);
    } catch (const std::exception& e) {
        spdlog::critical("OperationBlacks::execute: Unexpected Exception: {}", e.what());
        return std::unexpected(ErrorHandling::CoreError::Unexpected);
    }
}

// ============================================================================
// IOperationFusionLogic Implementation
// ============================================================================

Halide::Func OperationBlacks::appendToFusedPipeline(
    const Halide::Func& input_func,
    const Halide::Var& x,
    const Halide::Var& y,
    const Halide::Var& c,
    const OperationDescriptor& params
    ) const
{
    auto value_res = params.getParam<float>("value");
    float blacks_value = value_res.value_or(OperationBlacks::DEFAULT_BLACKS_VALUE);

    if (std::abs(blacks_value - OperationBlacks::DEFAULT_BLACKS_VALUE) < std::numeric_limits<float>::epsilon()) {
        spdlog::trace("OperationBlacks::appendToFusedPipeline: No-op requested, returning input");
        return input_func;
    }

    blacks_value = std::clamp(blacks_value, OperationBlacks::MIN_BLACKS_VALUE, OperationBlacks::MAX_BLACKS_VALUE);
    spdlog::debug("OperationBlacks::appendToFusedPipeline: Fusing with value={:.2f}", blacks_value);

    return applyBlacksAdjustment(input_func, blacks_value, x, y, c);
}

// ============================================================================
// IOperationDefaultLogic Implementation
// ============================================================================

std::expected<void, ErrorHandling::CoreError> OperationBlacks::executeOnImageRegion(
    Common::ImageRegion& region,
    const OperationDescriptor& params
    ) const
{
    if (!region.isValid()) {
        spdlog::error("[OperationBlacks] executeOnImageRegion: Invalid ImageRegion.");
        return std::unexpected(ErrorHandling::CoreError::InvalidImageRegion);
    }

    auto value_res = params.getParam<float>("value");
    if (!value_res) {
        spdlog::warn("[OperationBlacks] executeOnImageRegion: Param 'value' missing, skipping.");
        return {};
    }

    // TODO implement with OpenImageIO or OpenCV Or manually. To determine

    return {};
}

} // namespace CaptureMoment::Core::Operations
