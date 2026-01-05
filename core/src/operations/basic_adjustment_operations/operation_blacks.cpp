/**
 * @file operation_blacks.cpp
 * @brief Implementation of OperationBlacks
 * @author CaptureMoment Team
 * @date 2025
 */

#include "operations/basic_adjustment_operations/operation_blacks.h"
#include <spdlog/spdlog.h>
#include <Halide.h>

namespace CaptureMoment::Core::Operations {

bool OperationBlacks::execute(Common::ImageRegion& input, const OperationDescriptor& descriptor)
{
    // 1. Validation
    if (!input.isValid()) {
        spdlog::warn("OperationBlacks::execute: Invalid input region");
        return false;
    }

    if (!descriptor.enabled) {
        spdlog::trace("OperationBlacks::execute: Operation disabled, skipping");
        return true;
    }

    // 2. Extract parameter using key-value access
    // Retrieves "value" key with default 0.0f if missing or type mismatch
    const float blacks_value = descriptor.getParam<float>("value", 0.0f);

    // No-op optimization
    if (blacks_value == 0.0f) {
        spdlog::trace("OperationBlacks::execute: Blacks value is 0, skipping");
        return true;
    }

    spdlog::debug("OperationBlacks::execute: value={:.2f} on {}x{} ({}ch) region",
                  blacks_value, input.m_width, input.m_height, input.m_channels);

    // 3. Halide pipeline
    try {

        spdlog::info("OperationBlacks::execute: Creating Halide buffer");
        spdlog::info("Data size: {}", input.m_data.size());
        spdlog::info("Expected size: {}", input.m_width * input.m_height * input.m_channels);

        // Create Halide function
        Halide::Func blacks;
        Halide::Var x, y, c;

        // Create input image from buffer (direct access via x, y, c)
        Halide::Buffer<float> input_buf(
            input.m_data.data(),
            input.m_width,
            input.m_height,
            input.m_channels
            );

        spdlog::info("OperationBlacks::execute: Halide buffer created successfully");

        // Apply blacks adjustment: Modify pixels based on their luminance
        // A simple approach: Use a mask based on luminance to determine how much to adjust
        // Luminance approximation: 0.299*R + 0.587*G + 0.114*B
        Halide::Func luminance;
        luminance(x, y) = 0.299f * input_buf(x, y, 0) + 0.587f * input_buf(x, y, 1) + 0.114f * input_buf(x, y, 2);

        // Create a mask that increases the adjustment effect for very dark pixels
        // For example, a mask that is 1 for pixels below a low threshold and approaches 0 for brighter pixels
        // Using a smoothstep-like function: (high - x) / (high - low), clamped between 0 and 1
        // Here, low = 0.0, high = 0.3 (or lower, adjustable thresholds)
        const float low_threshold = 0.0f; // Min luminance
        const float high_threshold = 0.3f; // Only affect pixels up to this luminance

        Halide::Func mask;
        mask(x, y) = Halide::select(
            luminance(x, y) >= high_threshold,
            0.0f, // No adjustment for pixels above the threshold
            luminance(x, y) <= low_threshold,
            1.0f, // Full adjustment for pixels at minimum luminance
            (high_threshold - luminance(x, y)) / (high_threshold - low_threshold) // Smooth transition from min to threshold
            );

        // Apply the adjustment: Add (or subtract) the value, scaled by the mask
        // This primarily affects the darkest pixels (blacks) more than mid-tones
        blacks(x, y, c) = Halide::select(
            c < 3, // If channel is R, G, or B
            input_buf(x, y, c) + blacks_value * mask(x, y), // Apply adjustment scaled by mask
            input_buf(x, y, c) // Else, keep Alpha channel unchanged
            );

        // Schedule for parallel execution
        spdlog::info("OperationBlacks::execute: Halide function defined");
        blacks.parallel(y, 8).vectorize(x, 8);
        spdlog::info("OperationBlacks::execute: Schedule applied, about to realize");

        // Realize back into the original buffer
        blacks.realize(input_buf);
        spdlog::info("OperationBlacks::execute: Halide realize completed successfully");

        return true;

    } catch (const std::exception& e) {
        spdlog::critical("OperationBlacks::execute: Halide exception: {}", e.what());
        return false;
    }
}

} // namespace CaptureMoment::Core::Operations
