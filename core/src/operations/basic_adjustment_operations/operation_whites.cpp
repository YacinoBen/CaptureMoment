/**
 * @file operation_whites.cpp
 * @brief Implementation of OperationWhites
 * @author CaptureMoment Team
 * @date 2025
 */

#include "operations/basic_adjustment_operations/operation_whites.h"

#include <spdlog/spdlog.h>
#include <algorithm>

namespace CaptureMoment::Core::Operations {

template<typename InputType>
Halide::Func applyWhitesAdjustment(
    const InputType& input,
    float whites_value,
    const Halide::Var& x,
    const Halide::Var& y,
    const Halide::Var& c,
    float low_threshold = 0.7f,
    float high_threshold = 1.0f
    )
{
    Halide::Func whites_func("whites_op");
    Halide::Func luminance_func("luminance_whites");
    Halide::Func mask_func("mask_whites");

    // Calcul de la luminance en utilisant les variables passées en paramètre
    luminance_func(x, y) = 0.299f * input(x, y, 0) + 0.587f * input(x, y, 1) + 0.114f * input(x, y, 2);

    // Masque basé sur la luminance
    mask_func(x, y) = Halide::select(
        luminance_func(x, y) <= low_threshold,
        0.0f,
        luminance_func(x, y) >= high_threshold,
        1.0f,
        (luminance_func(x, y) - low_threshold) / (high_threshold - low_threshold)
        );

    // Application de l'ajustement blanc en utilisant les variables passées en paramètre
    whites_func(x, y, c) = Halide::select(
        c < 3, // R, G, B
        input(x, y, c) + whites_value * mask_func(x, y),
        input(x, y, c) // A (Alpha inchangé)
        );

    return whites_func;
}

bool OperationWhites::execute(ImageProcessing::IWorkingImageHardware& working_image, const OperationDescriptor& descriptor)
{
    // 1. Validate the input working image
    if (!working_image.isValid()) {
        spdlog::warn("OperationWhites::execute: Invalid working image provided");
        return false;
    }

    // Skip execution if the operation is disabled
    if (!descriptor.enabled) {
        spdlog::trace("OperationWhites::execute: Operation is disabled, skipping execution");
        return true;
    }

    // 2. Extract the white adjustment value parameter from the descriptor
    float whites_value = descriptor.getParam<float>("value", 0.0f);

    // No-op optimization: Skip processing if the value matches the default
    if (whites_value == OperationWhites::DEFAULT_WHITES_VALUE) {
        spdlog::trace("OperationWhites::execute: White adjustment value is default ({}), skipping operation", OperationWhites::DEFAULT_WHITES_VALUE);
        return true;
    }

    // Validate and clamp the white adjustment value to the defined operational range
    if (whites_value < OperationWhites::MIN_WHITES_VALUE || whites_value > OperationWhites::MAX_WHITES_VALUE) {
        spdlog::warn("OperationWhites::execute: White adjustment value ({}) is outside the valid range [{}, {}]. Clamping.",
                     whites_value, OperationWhites::MIN_WHITES_VALUE, OperationWhites::MAX_WHITES_VALUE);
        whites_value = std::clamp(whites_value, OperationWhites::MIN_WHITES_VALUE, OperationWhites::MAX_WHITES_VALUE);
    }

    spdlog::debug("OperationWhites::execute: Applying white adjustment with value={:.2f}", whites_value);

    // Export the current working image data to a CPU-accessible copy for processing
    auto cpu_copy = working_image.exportToCPUCopy();
    if (!cpu_copy) {
        spdlog::error("OperationWhites::execute: Failed to export working image to CPU copy for processing");
        return false;
    }

    // 3. Execute the Halide-based image processing pipeline
    try {
        spdlog::info("OperationWhites::execute: Creating Halide buffer for processing");

        // Log image dimensions for debugging
        spdlog::info("OperationWhites::execute: Processing image size: {}x{} ({} channels), total elements: {}",
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

        spdlog::info("OperationWhites::execute: Halide buffer created successfully");

        // Appliquer l'ajustement blanc en utilisant la fonction utilitaire
        auto whites_func = applyWhitesAdjustment(input_buf, whites_value, x, y, c);

        // Schedule the Halide function for optimized parallel execution
        spdlog::info("OperationWhites::execute: Halide function defined with luminance mask logic");
        whites_func.parallel(y, 8).vectorize(x, 8); // Parallelize over Y-axis, vectorize over X-axis for performance
        spdlog::info("OperationWhites::execute: Parallel and vectorization schedule applied, about to realize");

        // Execute the Halide pipeline and write the result back to the input buffer
        whites_func.realize(input_buf);
        spdlog::info("OperationWhites::execute: Halide realization completed successfully");

        // Update the working image with the processed CPU copy data
        return working_image.updateFromCPU(*cpu_copy);

    } catch (const std::exception& e) {
        spdlog::critical("OperationWhites::execute: Exception occurred during Halide processing: {}", e.what());
        return false;
    }
}

Halide::Func OperationWhites::appendToFusedPipeline(
    const Halide::Func& input_func,
    const Halide::Var& x,
    const Halide::Var& y,
    const Halide::Var& c,
    const OperationDescriptor& params
    ) const
{
    // Extract the white adjustment value parameter from the operation descriptor
    float whites_value = params.getParam<float>("value", 0.0f);

    // No-op optimization: Return the input function unchanged if the value is at default
    if (whites_value == OperationWhites::DEFAULT_WHITES_VALUE) {
        spdlog::trace("OperationWhites::appendToFusedPipeline: No-op requested, returning input function unchanged");
        return input_func;
    }

    // Validate and clamp the white adjustment value to the defined operational range
    if (whites_value < OperationWhites::MIN_WHITES_VALUE || whites_value > OperationWhites::MAX_WHITES_VALUE) {
        spdlog::warn("OperationWhites::appendToFusedPipeline: Clamping adjustment value to valid range [{}, {}]",
                     OperationWhites::MIN_WHITES_VALUE, OperationWhites::MAX_WHITES_VALUE);
        whites_value = std::clamp(whites_value, OperationWhites::MIN_WHITES_VALUE, OperationWhites::MAX_WHITES_VALUE);
    }

    spdlog::debug("OperationWhites::appendToFusedPipeline: Building fusion fragment with value={:.2f}", whites_value);

    return applyWhitesAdjustment(input_func, whites_value, x, y, c);
}

bool OperationWhites::executeOnImageRegion(Common::ImageRegion& region, const OperationDescriptor& params) const
{
    if (!region.isValid()) {
        spdlog::error("[OperationWhites] executeOnImageRegion: Invalid ImageRegion.");
        return false;
    }

    const float whites_value = params.params.whites;
    const size_t pixel_count = region.getDataSize();

    // Apply white level adjustment to each pixel
    // Note: This is a simplified linear adjustment. More complex curves might be used in practice.
    for (size_t i = 0; i < pixel_count; ++i)
    {
        // Adjust pixel value based on the white level parameter
        // Example: Shift the curve so pixels near 1.0 are affected
        // (Real formula might involve exponential or logarithmic adjustments)
        region.m_data[i] = std::clamp(region.m_data[i] + whites_value, 0.0f, 1.0f);
        // Note: Adding directly is a simplification. Real white adjustment is often more subtle.
        // A more precise formula might involve adjusting the slope towards 1.0.
    }

    // Optional: Alternative implementation using OpenCV (requires converting ImageRegion to cv::Mat)
    /*
    cv::Mat cv_region(region.m_height, region.m_width, CV_32FC(region.m_channels), region.m_data.data());
    // Apply OpenCV white adjustment (e.g., using cv::LUT or custom calculation)
    // cv::Mat mask = (cv_region > 0.8); // Example mask for whites
    // cv_region.setTo(cv_region + whites_value, mask); // Example adjustment (adjust as needed)
    // Convert back if necessary (data is already modified in-place via cv::Mat view)
    */

    spdlog::debug("[OperationWhites] Applied whites {} to {} pixels.", whites_value, pixel_count);
    return true;
}

} // namespace CaptureMoment::Core::Operations
