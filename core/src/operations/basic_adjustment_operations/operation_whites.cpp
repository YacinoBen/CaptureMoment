/**
 * @file operation_whites.cpp
 * @brief Implementation of OperationWhites
 * @author CaptureMoment Team
 * @date 2025
 */

#include "operations/basic_adjustment_operations/operation_whites.h"
#include <spdlog/spdlog.h>
#include <Halide.h>

namespace CaptureMoment::Core::Operations {

bool OperationWhites::execute(Common::ImageRegion& input, const OperationDescriptor& descriptor)
{
    // 1. Validation
    if (!input.isValid()) {
        spdlog::warn("OperationWhites::execute: Invalid input region");
        return false;
    }

    if (!descriptor.enabled) {
        spdlog::trace("OperationWhites::execute: Operation disabled, skipping");
        return true;
    }

    // 2. Extract parameter using key-value access
    // Retrieves "value" key with default 0.0f if missing or type mismatch
    float whites_value = descriptor.getParam<float>("value", 0.0f);

    // No-op optimization (using the default value from OperationRanges via static member)
    if (whites_value == OperationWhites::DEFAULT_WHITES_VALUE) { // Use the static member
        spdlog::trace("OperationWhites::execute: Whites value is default ({}), skipping", OperationWhites::DEFAULT_WHITES_VALUE);
        return true;
    }

    // Validate and clamp the value to the defined range (Business Logic - Core)
    // Clamping is chosen here to ensure the operation always runs with a valid value,
    // preventing potential issues from out-of-range inputs while logging the warning.
    if (whites_value < OperationWhites::MIN_WHITES_VALUE || whites_value > OperationWhites::MAX_WHITES_VALUE) {
        spdlog::warn("OperationWhites::execute: Whites value ({}) is outside the valid range [{}, {}]. Clamping.",
                     whites_value, OperationWhites::MIN_WHITES_VALUE, OperationWhites::MAX_WHITES_VALUE);
        whites_value = std::clamp(whites_value, OperationWhites::MIN_WHITES_VALUE, OperationWhites::MAX_WHITES_VALUE);
    }

    spdlog::debug("OperationWhites::execute: value={:.2f} on {}x{} ({}ch) region",
                  whites_value, input.m_width, input.m_height, input.m_channels);

    // 3. Halide pipeline
    try {

        spdlog::info("OperationWhites::execute: Creating Halide buffer");
        spdlog::info("Data size: {}", input.m_data.size());
        spdlog::info("Expected size: {}", input.m_width * input.m_height * input.m_channels);

        // Create Halide function
        Halide::Func whites;
        Halide::Var x, y, c;

        // Create input image from buffer (direct access via x, y, c)
        Halide::Buffer<float> input_buf(
            input.m_data.data(),
            input.m_width,
            input.m_height,
            input.m_channels
            );

        spdlog::info("OperationWhites::execute: Halide buffer created successfully");

        // Apply whites adjustment: Modify pixels based on their luminance
        // A simple approach: Use a mask based on luminance to determine how much to adjust
        // Luminance approximation: 0.299*R + 0.587*G + 0.114*B
        Halide::Func luminance;
        luminance(x, y) = 0.299f * input_buf(x, y, 0) + 0.587f * input_buf(x, y, 1) + 0.114f * input_buf(x, y, 2);

        // Create a mask that increases the adjustment effect for very bright pixels
        // For example, a mask that is 0 for pixels below a high threshold and approaches 1 for the brightest pixels
        // Using a smoothstep-like function: (x - low) / (high - low), clamped between 0 and 1
        // Here, low = 0.7 (or higher), high = 1.0 (adjustable thresholds)
        const float low_threshold = 0.7f; // Only affect pixels above this luminance
        const float high_threshold = 1.0f; // Max luminance

        Halide::Func mask;
        mask(x, y) = Halide::select(
            luminance(x, y) <= low_threshold,
            0.0f, // No adjustment for pixels below the threshold
            luminance(x, y) >= high_threshold,
            1.0f, // Full adjustment for pixels at maximum luminance
            (luminance(x, y) - low_threshold) / (high_threshold - low_threshold) // Smooth transition from threshold to max
            );

        // Apply the adjustment: Add (or subtract) the value, scaled by the mask
        // This primarily affects the brightest pixels (whites) more than mid-tones
        whites(x, y, c) = Halide::select(
            c < 3, // If channel is R, G, or B
            input_buf(x, y, c) + whites_value * mask(x, y), // Apply adjustment scaled by mask
            input_buf(x, y, c) // Else, keep Alpha channel unchanged
            );

        // Schedule for parallel execution
        spdlog::info("OperationWhites::execute: Halide function defined");
        whites.parallel(y, 8).vectorize(x, 8);
        spdlog::info("OperationWhites::execute: Schedule applied, about to realize");

        // Realize back into the original buffer
        whites.realize(input_buf);
        spdlog::info("OperationWhites::execute: Halide realize completed successfully");

        return true;

    } catch (const std::exception& e) {
        spdlog::critical("OperationWhites::execute: Halide exception: {}", e.what());
        return false;
    }
}

} // namespace CaptureMoment::Core::Operations
