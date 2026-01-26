/**
 * @file operation_brightness.cpp
 * @brief Implementation of OperationBrightness
 * @author CaptureMoment Team
 * @date 2025
 */

#include "operations/basic_adjustment_operations/operation_brightness.h"

#include <spdlog/spdlog.h>
#include <algorithm>

namespace CaptureMoment::Core::Operations {

template<typename InputType>
Halide::Func applyBrightnessAdjustment(
    const InputType& input,
    float brightness_value,
    const Halide::Var& x,
    const Halide::Var& y,
    const Halide::Var& c
    )
{
    Halide::Func brightness_func("brightness_op");

    // Application de l'ajustement de luminosité
    brightness_func(x, y, c) = Halide::select(
        c < 3, // R, G, B
        Halide::clamp(input(x, y, c) + brightness_value, 0.0f, 1.0f),
        input(x, y, c) // A (Alpha inchangé)
        );

    return brightness_func;
}

bool OperationBrightness::execute(ImageProcessing::IWorkingImageHardware& working_image, const OperationDescriptor& descriptor)
{
    // 1. Validate the input working image
    if (!working_image.isValid()) {
        spdlog::warn("OperationBrightness::execute: Invalid working image provided");
        return false;
    }

    // Skip execution if the operation is disabled
    if (!descriptor.enabled) {
        spdlog::trace("OperationBrightness::execute: Operation is disabled, skipping execution");
        return true;
    }

    // 2. Extract the brightness adjustment value parameter from the descriptor
    float brightness_value = descriptor.getParam<float>("value", 0.0f);

    // No-op optimization: Skip processing if the value matches the default
    if (brightness_value == OperationBrightness::DEFAULT_BRIGHTNESS_VALUE) {
        spdlog::trace("OperationBrightness::execute: Brightness adjustment value is default ({}), skipping operation", OperationBrightness::DEFAULT_BRIGHTNESS_VALUE);
        return true;
    }

    // Validate and clamp the brightness adjustment value to the defined operational range
    if (brightness_value < OperationBrightness::MIN_BRIGHTNESS_VALUE || brightness_value > OperationBrightness::MAX_BRIGHTNESS_VALUE) {
        spdlog::warn("OperationBrightness::execute: Brightness adjustment value ({}) is outside the valid range [{}, {}]. Clamping.",
                     brightness_value, OperationBrightness::MIN_BRIGHTNESS_VALUE, OperationBrightness::MAX_BRIGHTNESS_VALUE);
        brightness_value = std::clamp(brightness_value, OperationBrightness::MIN_BRIGHTNESS_VALUE, OperationBrightness::MAX_BRIGHTNESS_VALUE);
    }

    spdlog::debug("OperationBrightness::execute: Applying brightness adjustment with value={:.2f}", brightness_value);

    // Export the current working image data to a CPU-accessible copy for processing
    auto cpu_copy = working_image.exportToCPUCopy();
    if (!cpu_copy) {
        spdlog::error("OperationBrightness::execute: Failed to export working image to CPU copy for processing");
        return false;
    }

    // 3. Execute the Halide-based image processing pipeline
    try {
        spdlog::info("OperationBrightness::execute: Creating Halide buffer for processing");

        // Log image dimensions for debugging
        spdlog::info("OperationBrightness::execute: Processing image size: {}x{} ({} channels), total elements: {}",
                     working_image.getSize().first,
                     working_image.getSize().second,
                     working_image.getChannels(),
                     working_image.getDataSize());

        // Create Halide variables for the coordinate system
        Halide::Var x, y, c;

        // Create input image buffer from the exported CPU copy data
        Halide::Buffer<float> input_buf(
            cpu_copy->m_data.data(),
            static_cast<int>(cpu_copy->m_width),
            static_cast<int>(cpu_copy->m_height),
            static_cast<int>(cpu_copy->m_channels)
            );

        spdlog::info("OperationBrightness::execute: Halide buffer created successfully");

        // Appliquer l'ajustement de luminosité en utilisant la fonction utilitaire
        auto brightness_func = applyBrightnessAdjustment(input_buf, brightness_value, x, y, c);

        // Schedule the Halide function for optimized parallel execution
        spdlog::info("OperationBrightness::execute: Halide function defined with brightness logic");
        brightness_func.parallel(y, 8).vectorize(x, 8); // Parallelize over Y-axis, vectorize over X-axis for performance
        spdlog::info("OperationBrightness::execute: Parallel and vectorization schedule applied, about to realize");

        // Execute the Halide pipeline and write the result back to the input buffer
        brightness_func.realize(input_buf);
        spdlog::info("OperationBrightness::execute: Halide realization completed successfully");

        // Update the working image with the processed CPU copy data
        return working_image.updateFromCPU(*cpu_copy);

    } catch (const std::exception& e) {
        spdlog::critical("OperationBrightness::execute: Exception occurred during Halide processing: {}", e.what());
        return false;
    }
}

Halide::Func OperationBrightness::appendToFusedPipeline(
    const Halide::Func& input_func,
    const Halide::Var& x,
    const Halide::Var& y,
    const Halide::Var& c,
    const OperationDescriptor& params
    ) const
{
    // Extract the brightness adjustment value parameter from the operation descriptor
    float brightness_value = params.getParam<float>("value", 0.0f);

    // No-op optimization: Return the input function unchanged if the value is at default
    if (brightness_value == OperationBrightness::DEFAULT_BRIGHTNESS_VALUE) {
        spdlog::trace("OperationBrightness::appendToFusedPipeline: No-op requested, returning input function unchanged");
        return input_func;
    }

    // Validate and clamp the brightness adjustment value to the defined operational range
    if (brightness_value < OperationBrightness::MIN_BRIGHTNESS_VALUE || brightness_value > OperationBrightness::MAX_BRIGHTNESS_VALUE) {
        spdlog::warn("OperationBrightness::appendToFusedPipeline: Clamping adjustment value to valid range [{}, {}]",
                     OperationBrightness::MIN_BRIGHTNESS_VALUE, OperationBrightness::MAX_BRIGHTNESS_VALUE);
        brightness_value = std::clamp(brightness_value, OperationBrightness::MIN_BRIGHTNESS_VALUE, OperationBrightness::MAX_BRIGHTNESS_VALUE);
    }

    spdlog::debug("OperationBrightness::appendToFusedPipeline: Building fusion fragment with value={:.2f}", brightness_value);

    return applyBrightnessAdjustment(input_func, brightness_value, x, y, c);
}

bool OperationBrightness::executeOnImageRegion(Common::ImageRegion& region, const OperationDescriptor& params) const
{
    if (!region.isValid()) {
        spdlog::error("[OperationBrightness] executeOnImageRegion: Invalid ImageRegion.");
        return false;
    }

    const float brightness_value = params.params.brightness;
    const size_t pixel_count = region.getDataSize();

    // Apply brightness adjustment to each pixel
    for (size_t i = 0; i < pixel_count; ++i) {
        region.m_data[i] = std::clamp(region.m_data[i] + brightness_value, 0.0f, 1.0f);
    }

    // Optional: Alternative implementation using OpenCV (requires converting ImageRegion to cv::Mat)
    /*
    cv::Mat cv_region(region.m_height, region.m_width, CV_32FC(region.m_channels), region.m_data.data());
    // Apply OpenCV brightness adjustment (e.g., using cv::convertScaleAbs or cv::addWeighted)
    // cv::Mat adjusted_region;
    // cv::addWeighted(cv_region, 1.0, cv::Mat::zeros(cv_region.size(), cv_region.type()), 0.0, brightness_value, adjusted_region);
    // cv_region = adjusted_region; // Assign back to original data view
    */

    spdlog::debug("[OperationBrightness] Applied brightness {} to {} pixels.", brightness_value, pixel_count);
    return true;
}

} // namespace CaptureMoment::Core::Operations
