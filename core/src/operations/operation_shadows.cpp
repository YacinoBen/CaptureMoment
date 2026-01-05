/**
 * @file operation_shadows.cpp
 * @brief Implementation of OperationShadows
 * @author CaptureMoment Team
 * @date 2025
 */

#include "operations/operation_shadows.h"
#include <spdlog/spdlog.h>
#include <Halide.h>

namespace CaptureMoment::Core::Operations {

bool OperationShadows::execute(Common::ImageRegion& input, const OperationDescriptor& descriptor)
{
    // 1. Validation
    if (!input.isValid()) {
        spdlog::warn("OperationShadows::execute: Invalid input region");
        return false;
    }

    if (!descriptor.enabled) {
        spdlog::trace("OperationShadows::execute: Operation disabled, skipping");
        return true;
    }

    // 2. Extract parameter using key-value access
    // Retrieves "value" key with default 0.0f if missing or type mismatch
    const float shadows_value = descriptor.getParam<float>("value", 0.0f);

    // No-op optimization
    if (shadows_value == 0.0f) {
        spdlog::trace("OperationShadows::execute: Shadows value is 0, skipping");
        return true;
    }

    spdlog::debug("OperationShadows::execute: value={:.2f} on {}x{} ({}ch) region",
                  shadows_value, input.m_width, input.m_height, input.m_channels);

    // 3. Halide pipeline
    try {

        spdlog::info("OperationShadows::execute: Creating Halide buffer");
        spdlog::info("Data size: {}", input.m_data.size());
        spdlog::info("Expected size: {}", input.m_width * input.m_height * input.m_channels);

        // Create Halide function
        Halide::Func shadows;
        Halide::Var x, y, c;

        // Create input image from buffer (direct access via x, y, c)
        Halide::Buffer<float> input_buf(
            input.m_data.data(),
            input.m_width,
            input.m_height,
            input.m_channels
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

        return true;

    } catch (const std::exception& e) {
        spdlog::critical("OperationShadows::execute: Halide exception: {}", e.what());
        return false;
    }
}

} // namespace CaptureMoment::Core::Operations
