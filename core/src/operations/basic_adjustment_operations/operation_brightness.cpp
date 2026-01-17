/**
 * @file operation_brightness.cpp
 * @brief Implementation of OperationBrightness
 * @author CaptureMoment Team
 * @date 2025
 */

#include "operations/basic_adjustment_operations/operation_brightness.h"

#include <spdlog/spdlog.h>
#include <Halide.h>
#include <algorithm> // For std::clamp

namespace CaptureMoment::Core::Operations {

bool OperationBrightness::execute(ImageProcessing::IWorkingImageHardware& working_image, const OperationDescriptor& descriptor)
{
    // 1. Validation
    if (!working_image.isValid()) {
        spdlog::warn("OperationBrightness::execute: Invalid working image");
        return false;
    }

    if (!descriptor.enabled) {
        spdlog::trace("OperationBrightness::execute: Operation disabled, skipping");
        return true;
    }

    // 2. Extract parameter using key-value access
    // Retrieves "value" key with default 0.0f if missing or type mismatch
    float brightness_value { descriptor.getParam<float>("value", 0.0f) };

    // No-op optimization (using the default value from OperationRanges)
    if (brightness_value == OperationBrightness::DEFAULT_BRIGHTNESS_VALUE) { // Use the static member
        spdlog::trace("OperationBrightness::execute: Brightness value is default ({}), skipping", OperationBrightness::DEFAULT_BRIGHTNESS_VALUE);
        return true;
    }

    // 3. Validate and clamp the value to the defined range (Business Logic - Core)
    // Clamping is chosen here to ensure the operation always runs with a valid value,
    // preventing potential issues from out-of-range inputs while logging the warning.
    if (brightness_value < OperationBrightness::MIN_BRIGHTNESS_VALUE || brightness_value > OperationBrightness::MAX_BRIGHTNESS_VALUE) {
        spdlog::warn("OperationBrightness::execute: Brightness value ({}) is outside the valid range [{}, {}]. Clamping.",
                     brightness_value, OperationBrightness::MIN_BRIGHTNESS_VALUE, OperationBrightness::MAX_BRIGHTNESS_VALUE);
        brightness_value = std::clamp(brightness_value, OperationBrightness::MIN_BRIGHTNESS_VALUE, OperationBrightness::MAX_BRIGHTNESS_VALUE);
    }

    spdlog::debug("OperationBrightness::execute: value={:.2f}", brightness_value);

    auto cpu_copy = working_image.exportToCPUCopy();
    if (!cpu_copy) {
        spdlog::error("OperationBrightness::execute: Failed to get CPU copy of working image.");
        return false;
    }

    // 4. Halide pipeline

    try {
        spdlog::info("OperationBrightness::execute: Creating Halide buffer");

        spdlog::info("OperationBrightness::execute: Image size: {}x{} ({} ch), total elements: {}",
                     working_image.getSize().first,
                     working_image.getSize().second,
                     working_image.getChannels(),
                     working_image.getDataSize());

        // Create Halide function using the CPU copy
        Halide::Func brightness;
        Halide::Var x, y, c;

        Halide::Buffer<float> input_buf(
            cpu_copy->m_data.data(),
            cpu_copy->m_width,
            cpu_copy->m_height,
            cpu_copy->m_channels
            );

        // Apply brightness: add to RGB (channels 0-2), keep Alpha (channel 3)
        brightness(x, y, c) = Halide::select(
            c < 3,
            input_buf(x, y, c) + brightness_value,
            input_buf(x, y, c)
            );

        // Schedule for parallel execution
        brightness.parallel(y, 8).vectorize(x, 8);

        // Realize back into the CPU buffer
        brightness.realize(input_buf);

        spdlog::info("OperationBrightness::execute: Halide realize completed successfully");

        // Write the result back to the working image (the backend will handle CPU/GPU transfer)
        return working_image.updateFromCPU(*cpu_copy);

    } catch (const std::exception& e) {
        spdlog::critical("OperationBrightness::execute: Halide exception: {}", e.what());
        return false;
    }
}

} // namespace CaptureMoment::Core::Operations
