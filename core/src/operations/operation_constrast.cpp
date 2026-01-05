/**
 * @file operation_contrast.cpp
 * @brief Implementation of OperationContrast
 * @author CaptureMoment Team
 * @date 2025
 */

#include "operations/operation_contrast.h"
#include <spdlog/spdlog.h>
#include <Halide.h>

namespace CaptureMoment::Core::Operations {

bool OperationContrast::execute(Common::ImageRegion& input, const OperationDescriptor& descriptor)
{
    // 1. Validation
    if (!input.isValid()) {
        spdlog::warn("OperationContrast::execute: Invalid input region");
        return false;
    }

    if (!descriptor.enabled) {
        spdlog::trace("OperationContrast::execute: Operation disabled, skipping");
        return true;
    }

    // 2. Extract parameter using key-value access
    // Retrieves "value" key with default 0.0f if missing or type mismatch
    const float contrast_value = descriptor.getParam<float>("value", 0.0f);

    // No-op optimization
    if (contrast_value == 0.0f) {
        spdlog::trace("OperationContrast::execute: Contrast value is 0, skipping");
        return true;
    }

    spdlog::debug("OperationContrast::execute: value={:.2f} on {}x{} ({}ch) region",
                  contrast_value, input.m_width, input.m_height, input.m_channels);

    // 3. Halide pipeline
    try {

        spdlog::info("OperationContrast::execute: Creating Halide buffer");
        spdlog::info("Data size: {}", input.m_data.size());
        spdlog::info("Expected size: {}", input.m_width * input.m_height * input.m_channels);

        // Create Halide function
        Halide::Func contrast;
        Halide::Var x, y, c;

        // Create input image from buffer (direct access via x, y, c)
        Halide::Buffer<float> input_buf(
            input.m_data.data(),
            input.m_width,
            input.m_height,
            input.m_channels
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

        return true;

    } catch (const std::exception& e) {
        spdlog::critical("OperationContrast::execute: Halide exception: {}", e.what());
        return false;
    }
}

} // namespace CaptureMoment::Core::Operations
