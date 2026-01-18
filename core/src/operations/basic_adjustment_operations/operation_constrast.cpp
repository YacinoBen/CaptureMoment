/**
 * @file operation_contrast.cpp
 * @brief Implementation of OperationContrast
 * @author CaptureMoment Team
 * @date 2025
 */

#include "operations/basic_adjustment_operations/operation_contrast.h"
#include <spdlog/spdlog.h>
#include <Halide.h>

namespace CaptureMoment::Core::Operations {

bool OperationContrast::execute(ImageProcessing::IWorkingImageHardware& working_image, const OperationDescriptor& descriptor)
{
    // 1. Validation
    if (!working_image.isValid()) {
        spdlog::warn("OperationContrast::execute: Invalid  working_image");
        return false;
    }

    if (!descriptor.enabled) {
        spdlog::trace("OperationContrast::execute: Operation disabled, skipping");
        return true;
    }

    // 2. Extract parameter using key-value access
    // Retrieves "value" key with default 0.0f if missing or type mismatch
    float contrast_value = descriptor.getParam<float>("value", 0.0f);

    // No-op optimization (using the default value from OperationRanges via static member)
    if (contrast_value == OperationContrast::DEFAULT_CONTRAST_VALUE) { // Use the static member
        spdlog::trace("OperationContrast::execute: Contrast value is default ({}), skipping", OperationContrast::DEFAULT_CONTRAST_VALUE);
        return true;
    }

    // Validate and clamp the value to the defined range (Business Logic - Core)
    // Clamping is chosen here to ensure the operation always runs with a valid value,
    // preventing potential issues from out-of-range inputs while logging the warning.
    if (contrast_value < OperationContrast::MIN_CONTRAST_VALUE || contrast_value > OperationContrast::MAX_CONTRAST_VALUE) {
        spdlog::warn("OperationContrast::execute: Contrast value ({}) is outside the valid range [{}, {}]. Clamping.",
                     contrast_value, OperationContrast::MIN_CONTRAST_VALUE, OperationContrast::MAX_CONTRAST_VALUE);
        contrast_value = std::clamp(contrast_value, OperationContrast::MIN_CONTRAST_VALUE, OperationContrast::MAX_CONTRAST_VALUE);
    }

    spdlog::debug("OperationContrast::execute: value={:.2f}", contrast_value);

    auto cpu_copy = working_image.exportToCPUCopy();
    if (!cpu_copy) {
        spdlog::error("OperationContrast::execute: Failed to get CPU copy of working image.");
        return false;
    }

    // 3. Halide pipeline
    try {

        spdlog::info("OperationContrast::execute: Creating Halide buffer");

        spdlog::info("OperationContrast::execute: Image size: {}x{} ({} ch), total elements: {}",
                     working_image.getSize().first,
                     working_image.getSize().second,
                     working_image.getChannels(),
                     working_image.getDataSize());

        // Create Halide function
        Halide::Func contrast;
        Halide::Var x, y, c;

        // Create input image from buffer (direct access via x, y, c)
        Halide::Buffer<float> input_buf(
            cpu_copy->m_data.data(),
            cpu_copy->m_width,
            cpu_copy->m_height,
            cpu_copy->m_channels
            );

        spdlog::info("OperationContrast::execute: Halide buffer created successfully");

        // Apply contrast: scale pixel values around the midpoint (0.5)
        // Formula: p_c = 0.5 + (p_c - 0.5) * (1.0 + value)
        // This is applied to RGB channels (0, 1, 2), Alpha (channel 3) remains unchanged.
        contrast(x, y, c) = Halide::select(
            c < 3, // If channel is R, G, or B
            0.5f + (input_buf(x, y, c) - 0.5f) * (1.0f + contrast_value), // Apply contrast formula
            input_buf(x, y, c) // Else, keep Alpha channel unchanged
            );

        // Schedule for parallel execution
        spdlog::info("OperationContrast::execute: Halide function defined");
        contrast.parallel(y, 8).vectorize(x, 8); // Similar scheduling as brightness
        spdlog::info("OperationContrast::execute: Schedule applied, about to realize");

        // Realize back into the original buffer
        contrast.realize(input_buf);
        spdlog::info("OperationContrast::execute: Halide realize completed successfully");

        // Write the result back to the working image (the backend will handle CPU/GPU transfer)
        return working_image.updateFromCPU(*cpu_copy);
    } catch (const std::exception& e) {
        spdlog::critical("OperationContrast::execute: Halide exception: {}", e.what());
        return false;
    }
}

} // namespace CaptureMoment::Core::Operations
