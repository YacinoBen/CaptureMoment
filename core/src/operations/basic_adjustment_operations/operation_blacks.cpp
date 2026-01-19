/**
 * @file operation_blacks.cpp
 * @brief Implementation of OperationBlacks
 * @author CaptureMoment Team
 * @date 2025
 */

#include "operations/basic_adjustment_operations/operation_blacks.h"

#include <spdlog/spdlog.h>
#include <Halide.h>
#include <algorithm> // For std::clamp

namespace CaptureMoment::Core::Operations {

bool OperationBlacks::execute(ImageProcessing::IWorkingImageHardware& working_image, const OperationDescriptor& descriptor)
{
    // 1. Validate the input working image
    if (!working_image.isValid()) {
        spdlog::warn("OperationBlacks::execute: Invalid working image provided");
        return false;
    }

    // Skip execution if the operation is disabled
    if (!descriptor.enabled) {
        spdlog::trace("OperationBlacks::execute: Operation is disabled, skipping execution");
        return true;
    }

    // 2. Extract the black adjustment value parameter from the descriptor
    // Retrieves the "value" key with a default of 0.0f if missing or type mismatch occurs
    float blacks_value = descriptor.getParam<float>("value", 0.0f);

    // No-op optimization: Skip processing if the value matches the default
    if (blacks_value == OperationBlacks::DEFAULT_BLACKS_VALUE) {
        spdlog::trace("OperationBlacks::execute: Black adjustment value is default ({}), skipping operation", OperationBlacks::DEFAULT_BLACKS_VALUE);
        return true;
    }

    // Validate and clamp the black adjustment value to the defined operational range
    // This ensures the operation always runs with a valid value, preventing potential issues
    // from out-of-range inputs while logging a warning for invalid inputs
    if (blacks_value < OperationBlacks::MIN_BLACKS_VALUE || blacks_value > OperationBlacks::MAX_BLACKS_VALUE) {
        spdlog::warn("OperationBlacks::execute: Black adjustment value ({}) is outside the valid range [{}, {}]. Clamping.",
                     blacks_value, OperationBlacks::MIN_BLACKS_VALUE, OperationBlacks::MAX_BLACKS_VALUE);
        blacks_value = std::clamp(blacks_value, OperationBlacks::MIN_BLACKS_VALUE, OperationBlacks::MAX_BLACKS_VALUE);
    }

    spdlog::debug("OperationBlacks::execute: Applying black adjustment with value={:.2f}", blacks_value);

    // Export the current working image data to a CPU-accessible copy for processing
    auto cpu_copy = working_image.exportToCPUCopy();
    if (!cpu_copy) {
        spdlog::error("OperationBlacks::execute: Failed to export working image to CPU copy for processing");
        return false;
    }

    // 3. Execute the Halide-based image processing pipeline
    try {
        spdlog::info("OperationBlacks::execute: Creating Halide buffer for processing");

        // Log image dimensions for debugging
        spdlog::info("OperationBlacks::execute: Processing image size: {}x{} ({} channels), total elements: {}",
                     working_image.getSize().first,
                     working_image.getSize().second,
                     working_image.getChannels(),
                     working_image.getDataSize());

        // Create Halide variables for the coordinate system
        Halide::Var x, y, c;

        // Create input image buffer from the exported CPU copy data
        // The buffer is constructed directly from the raw data pointer of the ImageRegion
        Halide::Buffer<float> input_buf(
            cpu_copy->m_data.data(),
            cpu_copy->m_width,
            cpu_copy->m_height,
            cpu_copy->m_channels
        );

        spdlog::info("OperationBlacks::execute: Halide buffer created successfully");

        // Apply black level adjustment: Modify pixels based on their luminance
        // This uses a luminance-based mask to determine how much to adjust each pixel
        // Standard luminance approximation: 0.299*R + 0.587*G + 0.114*B

        // Create the main processing function for the black adjustment
        Halide::Func blacks;

        // Calculate luminance approximation for each pixel
        Halide::Func luminance;
        luminance(x, y) = 0.299f * input_buf(x, y, 0) + 0.587f * input_buf(x, y, 1) + 0.114f * input_buf(x, y, 2);

        // Create a mask that increases the adjustment effect for very dark pixels
        // This mask is 1 for pixels below a low threshold and approaches 0 for brighter pixels
        // Uses a smooth transition function: (high - luminance) / (high - low), clamped between 0 and 1
        // Configuration: low = 0.0, high = 0.3 (adjustable thresholds for sensitivity)
        const float low_threshold = 0.0f; // Minimum luminance threshold
        const float high_threshold = 0.3f; // Maximum luminance threshold for affecting pixels

        Halide::Func mask;
        mask(x, y) = Halide::select(
            luminance(x, y) >= high_threshold,
            0.0f, // No adjustment for pixels above the high threshold
            luminance(x, y) <= low_threshold,
            1.0f, // Full adjustment for pixels at minimum luminance
            (high_threshold - luminance(x, y)) / (high_threshold - low_threshold) // Smooth transition from low to high threshold
        );

        // Apply the black level adjustment: Add (or subtract) the value, scaled by the mask
        // This primarily affects the darkest pixels (blacks) more than mid-tones
        // Channels R, G, B (indices 0, 1, 2) are adjusted; Alpha channel (index 3) remains unchanged
        blacks(x, y, c) = Halide::select(
            c < 3, // If channel is Red, Green, or Blue
            input_buf(x, y, c) + blacks_value * mask(x, y), // Apply adjustment scaled by the luminance mask
            input_buf(x, y, c) // Else, keep the Alpha channel unchanged
        );

        // Schedule the Halide function for optimized parallel execution
        spdlog::info("OperationBlacks::execute: Halide function defined with luminance mask logic");
        blacks.parallel(y, 8).vectorize(x, 8); // Parallelize over Y-axis, vectorize over X-axis for performance
        spdlog::info("OperationBlacks::execute: Parallel and vectorization schedule applied, about to realize");

        // Execute the Halide pipeline and write the result back to the input buffer
        blacks.realize(input_buf);
        spdlog::info("OperationBlacks::execute: Halide realization completed successfully");

        // Update the working image with the processed CPU copy data
        return working_image.updateFromCPU(*cpu_copy);

    } catch (const std::exception& e) {
        spdlog::critical("OperationBlacks::execute: Exception occurred during Halide processing: {}", e.what());
        return false;
    }
}

// Implementation of IOperationFusionLogic interface method

Halide::Func OperationBlacks::appendToFusedPipeline(
    const Halide::Func& input_func,
    const OperationDescriptor& params
) const
{
    // Extract the black adjustment value parameter from the operation descriptor
    float blacks_value = params.getParam<float>("value", 0.0f);

    // No-op optimization: Return the input function unchanged if the value is at default
    if (blacks_value == OperationBlacks::DEFAULT_BLACKS_VALUE) {
        spdlog::trace("OperationBlacks::appendToFusedPipeline: No-op requested, returning input function unchanged");
        return input_func;
    }

    // Validate and clamp the black adjustment value to the defined operational range
    if (blacks_value < OperationBlacks::MIN_BLACKS_VALUE || blacks_value > OperationBlacks::MAX_BLACKS_VALUE) {
        spdlog::warn("OperationBlacks::appendToFusedPipeline: Clamping adjustment value to valid range [{}, {}]",
                     OperationBlacks::MIN_BLACKS_VALUE, OperationBlacks::MAX_BLACKS_VALUE);
        blacks_value = std::clamp(blacks_value, OperationBlacks::MIN_BLACKS_VALUE, OperationBlacks::MAX_BLACKS_VALUE);
    }

    spdlog::debug("OperationBlacks::appendToFusedPipeline: Building fusion fragment with value={:.2f}", blacks_value);

    // Create Halide variables for the coordinate system in the fused pipeline
    Halide::Var x, y, c;

    // Create the main processing function for the black adjustment in the fused pipeline
    Halide::Func blacks_func("blacks_op");
    Halide::Func luminance_func("luminance_blacks");
    Halide::Func mask_func("mask_blacks");

    // Calculate luminance approximation based on the input function
    // Standard luminance formula: 0.299*R + 0.587*G + 0.114*B
    luminance_func(x, y) = 0.299f * input_func(x, y, 0) + 0.587f * input_func(x, y, 1) + 0.114f * input_func(x, y, 2);

    // Create a luminance-based mask to control the adjustment intensity
    // This mask targets dark pixels for stronger adjustment
    const float low_threshold = 0.0f;   // Minimum luminance for full adjustment
    const float high_threshold = 0.3f;  // Maximum luminance for any adjustment

    mask_func(x, y) = Halide::select(
        luminance_func(x, y) >= high_threshold,
        0.0f, // No adjustment for bright pixels
        luminance_func(x, y) <= low_threshold,
        1.0f, // Full adjustment for very dark pixels
        (high_threshold - luminance_func(x, y)) / (high_threshold - low_threshold) // Smooth transition
    );

    // Apply the black level adjustment using the luminance mask
    // This affects RGB channels while leaving the alpha channel unchanged
    blacks_func(x, y, c) = Halide::select(
        c < 3, // Process RGB channels (0, 1, 2)
        input_func(x, y, c) + blacks_value * mask_func(x, y), // Apply masked adjustment
        input_func(x, y, c) // Keep alpha channel (index 3) unchanged
    );

    // Return the new function representing this operation in the fused pipeline
    // This function will be chained with other operations in the overall pipeline
    return blacks_func;
}

} // namespace CaptureMoment::Core::Operations
