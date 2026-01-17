/**
 * @file operation_shadows.cpp
 * @brief Implementation of OperationShadows
 * @author CaptureMoment Team
 * @date 2025
 */

#include "operations/basic_adjustment_operations/operation_shadows.h"
#include <spdlog/spdlog.h>
#include <Halide.h>

namespace CaptureMoment::Core::Operations {

bool OperationShadows::execute(ImageProcessing::IWorkingImageHardware& working_image, const OperationDescriptor& descriptor)
{
    // 1. Validation
    if (!working_image.isValid()) {
        spdlog::warn("OperationShadows::execute: Invalid working_image");
        return false;
    }

    if (!descriptor.enabled) {
        spdlog::trace("OperationShadows::execute: Operation disabled, skipping");
        return true;
    }

    // 2. Extract parameter using key-value access
    // Retrieves "value" key with default 0.0f if missing or type mismatch
    float shadows_value = descriptor.getParam<float>("value", 0.0f);

    // No-op optimization (using the default value from OperationRanges via static member)
    if (shadows_value == OperationShadows::DEFAULT_SHADOWS_VALUE) { // Use the static member
        spdlog::trace("OperationShadows::execute: Shadows value is default ({}), skipping", OperationShadows::DEFAULT_SHADOWS_VALUE);
        return true;
    }

    // Validate and clamp the value to the defined range (Business Logic - Core)
    // Clamping is chosen here to ensure the operation always runs with a valid value,
    // preventing potential issues from out-of-range inputs while logging the warning.
    if (shadows_value < OperationShadows::MIN_SHADOWS_VALUE || shadows_value > OperationShadows::MAX_SHADOWS_VALUE) {
        spdlog::warn("OperationShadows::execute: Shadows value ({}) is outside the valid range [{}, {}]. Clamping.",
                     shadows_value, OperationShadows::MIN_SHADOWS_VALUE, OperationShadows::MAX_SHADOWS_VALUE);
        shadows_value = std::clamp(shadows_value, OperationShadows::MIN_SHADOWS_VALUE, OperationShadows::MAX_SHADOWS_VALUE);
    }


    spdlog::debug("OperationShadows::execute: value={:.2f}", shadows_value);

    auto cpu_copy = working_image.exportToCPUCopy();
    if (!cpu_copy) {
        spdlog::error("OperationShadows::execute: Failed to get CPU copy of working image.");
        return false;
    }

    // 3. Halide pipeline
    try {

        spdlog::info("OperationShadows::execute: Creating Halide buffer");

        spdlog::info("OperationShadows::execute: Image size: {}x{} ({} ch), total elements: {}",
                      working_image.getSize().first,
                      working_image.getSize().second,
                      working_image.getChannels(),
                      working_image.getDataSize());

        // Create Halide function
        Halide::Func shadows;
        Halide::Var x, y, c;

        // Create input image from buffer (direct access via x, y, c)
        Halide::Buffer<float> input_buf(
            cpu_copy->m_data.data(),
            cpu_copy->m_width,
            cpu_copy->m_height,
            cpu_copy->m_channels
            );

        spdlog::info("OperationShadows::execute: Halide buffer created successfully");

        // Apply shadows adjustment: Modify pixels based on their luminance
        // A simple approach: Use a mask based on luminance to determine how much to adjust
        // Luminance approximation: 0.299*R + 0.587*G + 0.114*B
        Halide::Func luminance;
        luminance(x, y) = 0.299f * input_buf(x, y, 0) + 0.587f * input_buf(x, y, 1) + 0.114f * input_buf(x, y, 2);

        // Create a mask that increases the adjustment effect for darker pixels
        // For example, a mask that is 1 for dark pixels and approaches 0 for bright pixels
        // Using a smoothstep-like function: (high - x) / (high - low), clamped between 0 and 1
        // Here, low = 0.0, high = 0.5 (adjustable thresholds)
        const float low_threshold = 0.0f;
        const float high_threshold = 0.5f;

        Halide::Func mask;
        mask(x, y) = Halide::select(
            luminance(x, y) >= high_threshold,
            0.0f, // No adjustment for bright pixels
            luminance(x, y) <= low_threshold,
            1.0f, // Full adjustment for very dark pixels
            (high_threshold - luminance(x, y)) / (high_threshold - low_threshold) // Smooth transition
            );

        // Apply the adjustment: Add (or subtract) the value, scaled by the mask
        // This primarily affects darker pixels more than brighter ones
        shadows(x, y, c) = Halide::select(
            c < 3, // If channel is R, G, or B
            input_buf(x, y, c) + shadows_value * mask(x, y), // Apply adjustment scaled by mask
            input_buf(x, y, c) // Else, keep Alpha channel unchanged
            );

        // Schedule for parallel execution
        spdlog::info("OperationShadows::execute: Halide function defined");
        shadows.parallel(y, 8).vectorize(x, 8);
        spdlog::info("OperationShadows::execute: Schedule applied, about to realize");

        // Realize back into the original buffer
        shadows.realize(input_buf);
        spdlog::info("OperationShadows::execute: Halide realize completed successfully");


        // Write the result back to the working image (the backend will handle CPU/GPU transfer)
        return working_image.updateFromCPU(*cpu_copy);

    } catch (const std::exception& e) {
        spdlog::critical("OperationShadows::execute: Halide exception: {}", e.what());
        return false;
    }
}

} // namespace CaptureMoment::Core::Operations
