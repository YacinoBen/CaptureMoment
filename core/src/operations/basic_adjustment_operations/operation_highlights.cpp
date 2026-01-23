/**
 * @file operation_highlights.cpp
 * @brief Implementation of OperationHighlights
 * @author CaptureMoment Team
 * @date 2025
 */

#include "operations/basic_adjustment_operations/operation_highlights.h"

#include <spdlog/spdlog.h>
#include <algorithm>

namespace CaptureMoment::Core::Operations {

template<typename InputType>
Halide::Func applyHighlightsAdjustment(
    const InputType& input,
    float highlights_value,
    const Halide::Var& x,
    const Halide::Var& y,
    const Halide::Var& c,
    float low_threshold = 0.7f,
    float high_threshold = 1.0f
    )
{
    Halide::Func highlights_func("highlights_op");
    Halide::Func luminance_func("luminance_highlights");
    Halide::Func mask_func("mask_highlights");

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

    // Application de l'ajustement de hautes lumières en utilisant les variables passées en paramètre
    highlights_func(x, y, c) = Halide::select(
        c < 3, // R, G, B
        input(x, y, c) + highlights_value * mask_func(x, y),
        input(x, y, c) // A (Alpha inchangé)
        );

    return highlights_func;
}

bool OperationHighlights::execute(ImageProcessing::IWorkingImageHardware& working_image, const OperationDescriptor& descriptor)
{
    // 1. Validate the input working image
    if (!working_image.isValid()) {
        spdlog::warn("OperationHighlights::execute: Invalid working image provided");
        return false;
    }

    // Skip execution if the operation is disabled
    if (!descriptor.enabled) {
        spdlog::trace("OperationHighlights::execute: Operation is disabled, skipping execution");
        return true;
    }

    // 2. Extract the highlight adjustment value parameter from the descriptor
    float highlights_value = descriptor.getParam<float>("value", 0.0f);

    // No-op optimization: Skip processing if the value matches the default
    if (highlights_value == OperationHighlights::DEFAULT_HIGHLIGHTS_VALUE) {
        spdlog::trace("OperationHighlights::execute: Highlight adjustment value is default ({}), skipping operation", OperationHighlights::DEFAULT_HIGHLIGHTS_VALUE);
        return true;
    }

    // Validate and clamp the highlight adjustment value to the defined operational range
    if (highlights_value < OperationHighlights::MIN_HIGHLIGHTS_VALUE || highlights_value > OperationHighlights::MAX_HIGHLIGHTS_VALUE) {
        spdlog::warn("OperationHighlights::execute: Highlight adjustment value ({}) is outside the valid range [{}, {}]. Clamping.",
                     highlights_value, OperationHighlights::MIN_HIGHLIGHTS_VALUE, OperationHighlights::MAX_HIGHLIGHTS_VALUE);
        highlights_value = std::clamp(highlights_value, OperationHighlights::MIN_HIGHLIGHTS_VALUE, OperationHighlights::MAX_HIGHLIGHTS_VALUE);
    }

    spdlog::debug("OperationHighlights::execute: Applying highlight adjustment with value={:.2f}", highlights_value);

    // Export the current working image data to a CPU-accessible copy for processing
    auto cpu_copy = working_image.exportToCPUCopy();
    if (!cpu_copy) {
        spdlog::error("OperationHighlights::execute: Failed to export working image to CPU copy for processing");
        return false;
    }

    // 3. Execute the Halide-based image processing pipeline
    try {
        spdlog::info("OperationHighlights::execute: Creating Halide buffer for processing");

        // Log image dimensions for debugging
        spdlog::info("OperationHighlights::execute: Processing image size: {}x{} ({} channels), total elements: {}",
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

        spdlog::info("OperationHighlights::execute: Halide buffer created successfully");

        // Appliquer l'ajustement de hautes lumières en utilisant la fonction utilitaire
        auto highlights_func = applyHighlightsAdjustment(input_buf, highlights_value, x, y, c);

        // Schedule the Halide function for optimized parallel execution
        spdlog::info("OperationHighlights::execute: Halide function defined with luminance mask logic");
        highlights_func.parallel(y, 8).vectorize(x, 8); // Parallelize over Y-axis, vectorize over X-axis for performance
        spdlog::info("OperationHighlights::execute: Parallel and vectorization schedule applied, about to realize");

        // Execute the Halide pipeline and write the result back to the input buffer
        highlights_func.realize(input_buf);
        spdlog::info("OperationHighlights::execute: Halide realization completed successfully");

        // Update the working image with the processed CPU copy data
        return working_image.updateFromCPU(*cpu_copy);

    } catch (const std::exception& e) {
        spdlog::critical("OperationHighlights::execute: Exception occurred during Halide processing: {}", e.what());
        return false;
    }
}

Halide::Func OperationHighlights::appendToFusedPipeline(
    const Halide::Func& input_func,
    const Halide::Var& x,
    const Halide::Var& y,
    const Halide::Var& c,
    const OperationDescriptor& params
    ) const
{
    // Extract the highlight adjustment value parameter from the operation descriptor
    float highlights_value = params.getParam<float>("value", 0.0f);

    // No-op optimization: Return the input function unchanged if the value is at default
    if (highlights_value == OperationHighlights::DEFAULT_HIGHLIGHTS_VALUE) {
        spdlog::trace("OperationHighlights::appendToFusedPipeline: No-op requested, returning input function unchanged");
        return input_func;
    }

    // Validate and clamp the highlight adjustment value to the defined operational range
    if (highlights_value < OperationHighlights::MIN_HIGHLIGHTS_VALUE || highlights_value > OperationHighlights::MAX_HIGHLIGHTS_VALUE) {
        spdlog::warn("OperationHighlights::appendToFusedPipeline: Clamping adjustment value to valid range [{}, {}]",
                     OperationHighlights::MIN_HIGHLIGHTS_VALUE, OperationHighlights::MAX_HIGHLIGHTS_VALUE);
        highlights_value = std::clamp(highlights_value, OperationHighlights::MIN_HIGHLIGHTS_VALUE, OperationHighlights::MAX_HIGHLIGHTS_VALUE);
    }

    spdlog::debug("OperationHighlights::appendToFusedPipeline: Building fusion fragment with value={:.2f}", highlights_value);

    return applyHighlightsAdjustment(input_func, highlights_value, x, y, c);
}

} // namespace CaptureMoment::Core::Operations
