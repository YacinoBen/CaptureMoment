/**
 * @file brightness_operation.cpp
 * @brief Implementation of BrightnessOperation
 * @author CaptureMoment Team
 * @date 2025
 */

#include "operations/operation_brightness.h"
#include <spdlog/spdlog.h>
#include <Halide.h>

namespace CaptureMoment::Core::Operations {

bool OperationBrightness::execute(Common::ImageRegion& input, const OperationDescriptor& descriptor)
{
    // 1. Validation
    if (!input.isValid()) {
        spdlog::warn("OperationBrightness::execute: Invalid input region");
        return false;
    }

    if (!descriptor.enabled) {
        spdlog::trace("OperationBrightness::execute: Operation disabled, skipping");
        return true;
    }

    // 2. Extract parameter using key-value access
    // Retrieves "value" key with default 0.0f if missing or type mismatch
    const float brightness_value = descriptor.getParam<float>("value", 0.0f);

    // No-op optimization
    if (brightness_value == 0.0f) {
        spdlog::trace("OperationBrightness::execute: Brightness value is 0, skipping");
        return true;
    }

    spdlog::debug("OperationBrightness::execute: value={:.2f} on {}x{} ({}ch) region",
                  brightness_value, input.m_width, input.m_height, input.m_channels);

    // 3. Halide pipeline
    try {

        spdlog::info("OperationBrightness::execute: Creating Halide buffer");
        spdlog::info("Data size: {}", input.m_data.size());
        spdlog::info("Expected size: {}", input.m_width * input.m_height * input.m_channels);


        // Create Halide function
        Halide::Func brightness;
        Halide::Var x, y, c;

        // Create input image from buffer (direct access via x, y, c)
        Halide::Buffer<float> input_buf(
            input.m_data.data(),
            input.m_width,
            input.m_height,
            input.m_channels
        );

        // Apply brightness: add to RGB (channels 0-2), keep Alpha (channel 3)
        spdlog::info("OperationBrightness::execute: Halide buffer created successfully");

        brightness(x, y, c) = Halide::select(
            c < 3,
            input_buf(x, y, c) + brightness_value,
            input_buf(x, y, c)
        );

        // Schedule for parallel execution
        spdlog::info("OperationBrightness::execute: Halide function defined");
        brightness.parallel(y, 8).vectorize(x, 8);
        spdlog::info("OperationBrightness::execute: Schedule applied, about to realize");

        // Realize back into the original buffer
        brightness.realize(input_buf);
        spdlog::info("OperationBrightness::execute: Halide realize completed successfully");

        return true;

    } catch (const std::exception& e) {
        spdlog::critical("OperationBrightness::execute: Halide exception: {}", e.what());
        return false;
    }
}

} // namespace CaptureMoment::Core::Operations
