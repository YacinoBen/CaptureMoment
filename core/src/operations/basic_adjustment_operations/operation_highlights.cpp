/**
 * @file operation_highlights.cpp
 * @brief Implementation of OperationHighlights
 * @author CaptureMoment Team
 * @date 2025
 */

#include "operations/basic_adjustment_operations/operation_highlights.h"
#include <spdlog/spdlog.h>
#include <Halide.h>

namespace CaptureMoment::Core::Operations {

bool OperationHighlights::execute(ImageProcessing::IWorkingImageHardware& working_image,  const OperationDescriptor& descriptor)
{
    // 1. Validation
    if (!working_image.isValid()) {
        spdlog::warn("OperationHighlights::execute: Invalid working_image");
        return false;
    }

    if (!descriptor.enabled) {
        spdlog::trace("OperationHighlights::execute: Operation disabled, skipping");
        return true;
    }

    // 2. Extract parameter using key-value access
    // Retrieves "value" key with default 0.0f if missing or type mismatch
    float highlights_value = descriptor.getParam<float>("value", 0.0f);

    // No-op optimization (using the default value from OperationRanges via static member)
    if (highlights_value == OperationHighlights::DEFAULT_HIGHLIGHTS_VALUE) { // Use the static member
        spdlog::trace("OperationHighlights::execute: Highlights value is default ({}), skipping", OperationHighlights::DEFAULT_HIGHLIGHTS_VALUE);
        return true;
    }

    // Validate and clamp the value to the defined range (Business Logic - Core)
    // Clamping is chosen here to ensure the operation always runs with a valid value,
    // preventing potential issues from out-of-range inputs while logging the warning.
    if (highlights_value < OperationHighlights::MIN_HIGHLIGHTS_VALUE || highlights_value > OperationHighlights::MAX_HIGHLIGHTS_VALUE) {
        spdlog::warn("OperationHighlights::execute: Highlights value ({}) is outside the valid range [{}, {}]. Clamping.",
                     highlights_value, OperationHighlights::MIN_HIGHLIGHTS_VALUE, OperationHighlights::MAX_HIGHLIGHTS_VALUE);
        highlights_value = std::clamp(highlights_value, OperationHighlights::MIN_HIGHLIGHTS_VALUE, OperationHighlights::MAX_HIGHLIGHTS_VALUE);
    }

    spdlog::debug("OperationHighlights::execute: value={:.2f}", highlights_value);

    auto cpu_copy = working_image.exportToCPUCopy();
    if (!cpu_copy) {
        spdlog::error("OperationHighlights::execute: Failed to get CPU copy of working image.");
        return false;
    }

    // 3. Halide pipeline
    try {

        spdlog::info("OperationHighlights::execute: Creating Halide buffer");

        spdlog::info("OperationHightlights::execute: Image size: {}x{} ({} ch), total elements: {}",
                     working_image.getSize().first,
                     working_image.getSize().second,
                     working_image.getChannels(),
                     working_image.getDataSize());

        // Create Halide function
        Halide::Func highlights;
        Halide::Var x, y, c;

        // Create input image from buffer (direct access via x, y, c)
        Halide::Buffer<float> input_buf(
            cpu_copy->m_data.data(),
            cpu_copy->m_width,
            cpu_copy->m_height,
            cpu_copy->m_channels
            );

        spdlog::info("OperationHighlights::execute: Halide buffer created successfully");

        // Apply highlights adjustment: Modify pixels based on their luminance
        // A simple approach: Use a mask based on luminance to determine how much to adjust
        // Luminance approximation: 0.299*R + 0.587*G + 0.114*B
        Halide::Func luminance;
        luminance(x, y) = 0.299f * input_buf(x, y, 0) + 0.587f * input_buf(x, y, 1) + 0.114f * input_buf(x, y, 2);

        // Create a mask that increases the adjustment effect for brighter pixels
        // For example, a mask that is 0 for dark pixels and approaches 1 for bright pixels
        // Using a smoothstep-like function: (x - low) / (high - low), clamped between 0 and 1
        // Here, low = 0.2, high = 0.8 (adjustable thresholds)
        const float low_threshold = 0.2f;
        const float high_threshold = 0.8f;

        Halide::Func mask;
        mask(x, y) = Halide::select(
            luminance(x, y) <= low_threshold,
            0.0f, // No adjustment for very dark pixels
            luminance(x, y) >= high_threshold,
            1.0f, // Full adjustment for very bright pixels
            (luminance(x, y) - low_threshold) / (high_threshold - low_threshold) // Smooth transition
            );

        // Apply the adjustment: Add (or subtract) the value, scaled by the mask
        // This primarily affects brighter pixels more than darker ones
        highlights(x, y, c) = Halide::select(
            c < 3, // If channel is R, G, or B
            input_buf(x, y, c) + highlights_value * mask(x, y), // Apply adjustment scaled by mask
            input_buf(x, y, c) // Else, keep Alpha channel unchanged
            );

        // Schedule for parallel execution
        spdlog::info("OperationHighlights::execute: Halide function defined");
        highlights.parallel(y, 8).vectorize(x, 8);
        spdlog::info("OperationHighlights::execute: Schedule applied, about to realize");

        // Realize back into the original buffer
        highlights.realize(input_buf);
        spdlog::info("OperationHighlights::execute: Halide realize completed successfully");

        // Write the result back to the working image (the backend will handle CPU/GPU transfer)
        return working_image.updateFromCPU(*cpu_copy);
    } catch (const std::exception& e) {
        spdlog::critical("OperationHighlights::execute: Halide exception: {}", e.what());
        return false;
    }
}

} // namespace CaptureMoment::Core::Operations
