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

bool OperationHighlights::execute(Common::ImageRegion& input, const OperationDescriptor& descriptor)
{
    // 1. Validation
    if (!input.isValid()) {
        spdlog::warn("OperationHighlights::execute: Invalid input region");
        return false;
    }

    if (!descriptor.enabled) {
        spdlog::trace("OperationHighlights::execute: Operation disabled, skipping");
        return true;
    }

    // 2. Extract parameter using key-value access
    // Retrieves "value" key with default 0.0f if missing or type mismatch
    const float highlights_value = descriptor.getParam<float>("value", 0.0f);

    // No-op optimization
    if (highlights_value == 0.0f) {
        spdlog::trace("OperationHighlights::execute: Highlights value is 0, skipping");
        return true;
    }

    spdlog::debug("OperationHighlights::execute: value={:.2f} on {}x{} ({}ch) region",
                  highlights_value, input.m_width, input.m_height, input.m_channels);

    // 3. Halide pipeline
    try {

        spdlog::info("OperationHighlights::execute: Creating Halide buffer");
        spdlog::info("Data size: {}", input.m_data.size());
        spdlog::info("Expected size: {}", input.m_width * input.m_height * input.m_channels);

        // Create Halide function
        Halide::Func highlights;
        Halide::Var x, y, c;

        // Create input image from buffer (direct access via x, y, c)
        Halide::Buffer<float> input_buf(
            input.m_data.data(),
            input.m_width,
            input.m_height,
            input.m_channels
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

        return true;

    } catch (const std::exception& e) {
        spdlog::critical("OperationHighlights::execute: Halide exception: {}", e.what());
        return false;
    }
}

} // namespace CaptureMoment::Core::Operations
