/**
 * @file operation_brightness.cpp
 * @brief Implementation of OperationBrightness
 * @author CaptureMoment Team
 * @date 2025
 */

#include "operations/basic_adjustment_operations/operation_brightness.h"
#include "common/error_handling/core_error.h"

#include <spdlog/spdlog.h>
#include <algorithm>
#include <limits>

namespace CaptureMoment::Core::Operations {

// ============================================================================
// Internal Helper: Shared Halide Logic
// ============================================================================

template<typename InputType>
Halide::Func applyBrightnessAdjustment(
    const InputType& input,
    float brightness_value,
    const Halide::Var& x,
    const Halide::Var& y,
    const Halide::Var& c)
{
    Halide::Func brightness_func("brightness_op");

    // Additive brightness adjustment
    brightness_func(x, y, c) = Halide::select(
        c < 3, // R, G, B
        Halide::clamp(input(x, y, c) + brightness_value, 0.0f, 1.0f),
        input(x, y, c) // Alpha unchanged
        );

    return brightness_func;
}

// ============================================================================
// IOperation Implementation
// ============================================================================

std::expected<void, ErrorHandling::CoreError> OperationBrightness::execute(
    ImageProcessing::IWorkingImageHardware& working_image,
    const OperationDescriptor& descriptor)
{
    // Step 1: Validation
    if (!working_image.isValid()) {
        spdlog::warn("OperationBrightness::execute: Invalid working image provided");
        return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
    }

    if (!descriptor.enabled) {
        spdlog::trace("OperationBrightness::execute: Operation is disabled, skipping");
        return {};
    }

    // Step 2: Extract Parameters
    auto value_res = descriptor.getParam<float>("value");
    if (!value_res) {
        spdlog::error("OperationBrightness::execute: Failed to get 'value' parameter");
        return std::unexpected(ErrorHandling::CoreError::Unexpected);
    }
    float brightness_value = value_res.value();

    // Step 3: No-Op Optimization
    if (std::abs(brightness_value - OperationBrightness::DEFAULT_BRIGHTNESS_VALUE) < std::numeric_limits<float>::epsilon()) {
        spdlog::trace("OperationBrightness::execute: Value is default, skipping");
        return {};
    }

    // Step 4: Clamp Value
    brightness_value = std::clamp(brightness_value, OperationBrightness::MIN_BRIGHTNESS_VALUE, OperationBrightness::MAX_BRIGHTNESS_VALUE);
    spdlog::debug("OperationBrightness::execute: Applying brightness with value={:.2f}", brightness_value);

    // Step 5: Export & Execute
    auto cpu_copy_result = working_image.exportToCPUCopy();
    if (!cpu_copy_result) {
        spdlog::error("OperationBrightness::execute: Failed to export working image to CPU");
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

        auto brightness_func = applyBrightnessAdjustment(input_buf, brightness_value, x, y, c);
        brightness_func.compute_root().parallel(y).vectorize(x, 8);
        brightness_func.realize(input_buf);

        auto update_res = working_image.updateFromCPU(std::move(*cpu_region_ptr));
        if (!update_res) {
            spdlog::error("OperationBrightness::execute: Failed to update working image from CPU");
            return std::unexpected(update_res.error());
        }

        return {};

    } catch (const std::exception& e) {
        spdlog::critical("OperationBrightness::execute: Exception: {}", e.what());
        return std::unexpected(ErrorHandling::CoreError::Unexpected);
    }
}

// ============================================================================
// IOperationFusionLogic Implementation
// ============================================================================

Halide::Func OperationBrightness::appendToFusedPipeline(
    const Halide::Func& input_func,
    const Halide::Var& x,
    const Halide::Var& y,
    const Halide::Var& c,
    const OperationDescriptor& params
    ) const
{
    auto value_res = params.getParam<float>("value");
    float brightness_value = value_res.value_or(OperationBrightness::DEFAULT_BRIGHTNESS_VALUE);

    if (std::abs(brightness_value - OperationBrightness::DEFAULT_BRIGHTNESS_VALUE) < std::numeric_limits<float>::epsilon()) {
        return input_func;
    }

    brightness_value = std::clamp(brightness_value, OperationBrightness::MIN_BRIGHTNESS_VALUE, OperationBrightness::MAX_BRIGHTNESS_VALUE);
    return applyBrightnessAdjustment(input_func, brightness_value, x, y, c);
}

// ============================================================================
// IOperationDefaultLogic Implementation
// ============================================================================

std::expected<void, ErrorHandling::CoreError> OperationBrightness::executeOnImageRegion(
    Common::ImageRegion& region,
    const OperationDescriptor& params
    ) const
{
    if (!region.isValid()) {
        spdlog::error("[OperationBrightness] executeOnImageRegion: Invalid ImageRegion.");
        return std::unexpected(ErrorHandling::CoreError::InvalidImageRegion);
    }

    auto value_res = params.getParam<float>("value");
    if (!value_res) {
        spdlog::warn("[OperationBrightness] executeOnImageRegion: Param 'value' missing, skipping.");
        return {};
    }

    // TODO implement with OpenImageIO or OpenCV Or manually. To determine

    return {};
}

} // namespace CaptureMoment::Core::Operations
