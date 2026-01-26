/**
 * @file operation_blacks.cpp
 * @brief Implementation of OperationBlacks
 * @author CaptureMoment Team
 * @date 2025
 */

#include "operations/basic_adjustment_operations/operation_blacks.h"

#include <spdlog/spdlog.h>
#include <algorithm>

namespace CaptureMoment::Core::Operations {
template<typename InputType>
Halide::Func applyBlacksAdjustment(
    const InputType& input,
    float blacks_value,
    const Halide::Var& x,
    const Halide::Var& y,
    const Halide::Var& c,
    float low_threshold = 0.0f,
    float high_threshold = 0.3f ) 
{
    Halide::Func blacks_func("blacks_op");
    Halide::Func luminance_func("luminance_blacks");
    Halide::Func mask_func("mask_blacks");

    // Calcul de la luminance en utilisant les variables passées en paramètre
    luminance_func(x, y) = 0.299f * input(x, y, 0) + 0.587f * input(x, y, 1) + 0.114f * input(x, y, 2);

    // Masque basé sur la luminance
    mask_func(x, y) = Halide::select(
        luminance_func(x, y) >= high_threshold,
        0.0f,
        luminance_func(x, y) <= low_threshold,
        1.0f,
        (high_threshold - luminance_func(x, y)) / (high_threshold - low_threshold)
    );

    // Application de l'ajustement noir en utilisant les variables passées en paramètre
    blacks_func(x, y, c) = Halide::select(
        c < 3, // R, G, B
        input(x, y, c) + blacks_value * mask_func(x, y),
        input(x, y, c) // A (Alpha inchangé)
    );

    return blacks_func;
}

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
            static_cast<int>(cpu_copy->m_width),
            static_cast<int>(cpu_copy->m_height),
            static_cast<int>(cpu_copy->m_channels)
        );

       spdlog::info("OperationBlacks::execute: Halide buffer created successfully");

        // Appliquer l'ajustement noir en utilisant la fonction utilitaire
        auto blacks_func = applyBlacksAdjustment(input_buf, blacks_value, x, y, c);

        // Schedule the Halide function for optimized parallel execution
        spdlog::info("OperationBlacks::execute: Halide function defined with luminance mask logic");
        blacks_func.parallel(y, 8).vectorize(x, 8); // Parallelize over Y-axis, vectorize over X-axis for performance
        spdlog::info("OperationBlacks::execute: Parallel and vectorization schedule applied, about to realize");

        // Execute the Halide pipeline and write the result back to the input buffer
        blacks_func.realize(input_buf);
        spdlog::info("OperationBlacks::execute: Halide realization completed successfully");

        // Update the working image with the processed CPU copy data
        return working_image.updateFromCPU(*cpu_copy);

    } catch (const std::exception& e) {
        spdlog::critical("OperationBlacks::execute: Exception occurred during Halide processing: {}", e.what());
        return false;
    }
}

Halide::Func OperationBlacks::appendToFusedPipeline(
    const Halide::Func& input_func,
    const Halide::Var& x,
    const Halide::Var& y,
    const Halide::Var& c,
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

    return applyBlacksAdjustment(input_func, blacks_value, x, y, c);
}

bool OperationBlacks::executeOnImageRegion(Common::ImageRegion& region, const OperationDescriptor& params) const
{
    if (!region.isValid()) {
        spdlog::error("[OperationBlacks] executeOnImageRegion: Invalid ImageRegion.");
        return false;
    }

    const float blacks_value = params.params.blacks;
    const size_t pixel_count = region.getDataSize();

    // Apply black level adjustment to each pixel
    // Note: This is a simplified linear adjustment. More complex curves might be used in practice.
    for (size_t i = 0; i < pixel_count; ++i) {
        // Adjust pixel value based on the black level parameter
        // Example: Shift the curve so pixels near 0.0 are affected
        // (Real formula might be more complex, e.g., involving gamma correction)
        region.m_data[i] = std::clamp(blacks_value + (region.m_data[i] * (1.0f - blacks_value)), 0.0f, 1.0f);
    }

    // Optional: Alternative implementation using OpenCV (requires converting ImageRegion to cv::Mat)
    /*
    cv::Mat cv_region(region.m_height, region.m_width, CV_32FC(region.m_channels), region.m_data.data());
    // Apply OpenCV adjustment (e.g., using cv::LUT or custom LUT calculation)
    // cv::convertScaleAbs(cv_region, cv_region, 255.0, -blacks_value * 255.0); // Example (adjust as needed)
    // Convert back if necessary (data is already modified in-place via cv::Mat view)
    */

    spdlog::debug("[OperationBlacks] Applied blacks {} to {} pixels.", blacks_value, pixel_count);
    return true;
}

} // namespace CaptureMoment::Core::Operations
