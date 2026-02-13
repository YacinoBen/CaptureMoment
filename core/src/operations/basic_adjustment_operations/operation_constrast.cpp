/**
 * @file operation_contrast.cpp
 * @brief Implementation of OperationContrast
 * @author CaptureMoment Team
 * @date 2025
 */

#include "operations/basic_adjustment_operations/operation_contrast.h"
#include "common/error_handling/core_error.h"

#include <spdlog/spdlog.h>
#include <algorithm>
#include <limits>

namespace CaptureMoment::Core::Operations {

// ============================================================================
// Internal Helper: Shared Halide Logic
// ============================================================================

template<typename InputType>
Halide::Func applyContrastAdjustment(
    const InputType& input,
    float contrast_value,
    const Halide::Var& x,
    const Halide::Var& y,
    const Halide::Var& c)
{
    Halide::Func contrast_func("contrast_op");

    // Multiplicative contrast centered at 0.5 (Mid-gray)
    // Formula: 0.5 + (Input - 0.5) * ContrastFactor
    contrast_func(x, y, c) = Halide::select(
        c < 3,
        Halide::clamp(0.5f + (input(x, y, c) - 0.5f) * contrast_value, 0.0f, 1.0f),
        input(x, y, c) // Alpha unchanged
        );

    return contrast_func;
}

// ============================================================================
// IOperation Implementation
// ============================================================================

std::expected<void, ErrorHandling::CoreError> OperationContrast::execute(
    ImageProcessing::IWorkingImageHardware& working_image,
    const OperationDescriptor& descriptor)
{
    // Step 1: Validation
    if (!working_image.isValid()) {
        spdlog::warn("OperationContrast::execute: Invalid working image provided");
        return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
    }

    if (!descriptor.enabled) {
        spdlog::trace("OperationContrast::execute: Operation is disabled, skipping");
        return {};
    }

    // Step 2: Extract Parameters
    auto value_res = descriptor.getParam<float>("value");
    if (!value_res) {
        spdlog::error("OperationContrast::execute: Failed to get 'value' parameter");
        return std::unexpected(ErrorHandling::CoreError::Unexpected);
    }
    float contrast_value = value_res.value();

    // Step 3: No-Op Optimization
    if (std::abs(contrast_value - OperationContrast::DEFAULT_CONTRAST_VALUE) < std::numeric_limits<float>::epsilon()) {
        spdlog::trace("OperationContrast::execute: Value is default, skipping");
        return {};
    }

    // Step 4: Clamp Value
    contrast_value = std::clamp(contrast_value, OperationContrast::MIN_CONTRAST_VALUE, OperationContrast::MAX_CONTRAST_VALUE);
    spdlog::debug("OperationContrast::execute: Applying contrast with value={:.2f}", contrast_value);

    // Step 5: Export & Execute
    auto cpu_copy_result = working_image.exportToCPUCopy();
    if (!cpu_copy_result) {
        spdlog::error("OperationContrast::execute: Failed to export working image to CPU");
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

        auto contrast_func = applyContrastAdjustment(input_buf, contrast_value, x, y, c);
        contrast_func.compute_root().parallel(y).vectorize(x, 8);
        contrast_func.realize(input_buf);

        auto update_res = working_image.updateFromCPU(std::move(*cpu_region_ptr));
        if (!update_res) {
            spdlog::error("OperationContrast::execute: Failed to update working image from CPU");
            return std::unexpected(update_res.error());
        }

        return {};

    } catch (const std::exception& e) {
        spdlog::critical("OperationContrast::execute: Exception: {}", e.what());
        return std::unexpected(ErrorHandling::CoreError::Unexpected);
    }
}

// ============================================================================
// IOperationFusionLogic Implementation
// ============================================================================

Halide::Func OperationContrast::appendToFusedPipeline(
    const Halide::Func& input_func,
    const Halide::Var& x,
    const Halide::Var& y,
    const Halide::Var& c,
    const OperationDescriptor& params
    ) const
{
    auto value_res = params.getParam<float>("value");
    float contrast_value = value_res.value_or(OperationContrast::DEFAULT_CONTRAST_VALUE);

    if (std::abs(contrast_value - OperationContrast::DEFAULT_CONTRAST_VALUE) < std::numeric_limits<float>::epsilon()) {
        return input_func;
    }

    contrast_value = std::clamp(contrast_value, OperationContrast::MIN_CONTRAST_VALUE, OperationContrast::MAX_CONTRAST_VALUE);
    return applyContrastAdjustment(input_func, contrast_value, x, y, c);
}

// ============================================================================
// IOperationDefaultLogic Implementation
// ============================================================================

std::expected<void, ErrorHandling::CoreError> OperationContrast::executeOnImageRegion(
    Common::ImageRegion& region,
    const OperationDescriptor& params
    ) const
{
    if (!region.isValid()) {
        spdlog::error("[OperationContrast] executeOnImageRegion: Invalid ImageRegion.");
        return std::unexpected(ErrorHandling::CoreError::InvalidImageRegion);
    }

    auto value_res = params.getParam<float>("value");
    if (!value_res) {
        spdlog::warn("[OperationContrast] executeOnImageRegion: Param 'value' missing, skipping.");
        return {};
    }

    // TODO implement with OpenImageIO or OpenCV Or manually. To determine

    return {};
}

} // namespace CaptureMoment::Core::Operations
