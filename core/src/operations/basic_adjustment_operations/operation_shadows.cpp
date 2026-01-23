/**
 * @file operation_shadows.cpp
 * @brief Implementation of OperationShadows
 * @author CaptureMoment Team
 * @date 2025
 */

#include "operations/basic_adjustment_operations/operation_shadows.h"

#include <spdlog/spdlog.h>
#include <algorithm>

namespace CaptureMoment::Core::Operations {

template<typename InputType>
Halide::Func applyShadowsAdjustment(
    const InputType& input,
    float shadows_value,
    const Halide::Var& x,
    const Halide::Var& y,
    const Halide::Var& c,
    float low_threshold = 0.0f,
    float high_threshold = 0.3f
    )
{
    Halide::Func shadows_func("shadows_op");
    Halide::Func luminance_func("luminance_shadows");
    Halide::Func mask_func("mask_shadows");

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

    // Application de l'ajustement d'ombres en utilisant les variables passées en paramètre
    shadows_func(x, y, c) = Halide::select(
        c < 3, // R, G, B
        input(x, y, c) + shadows_value * mask_func(x, y),
        input(x, y, c) // A (Alpha inchangé)
        );

    return shadows_func;
}

bool OperationShadows::execute(ImageProcessing::IWorkingImageHardware& working_image, const OperationDescriptor& descriptor)
{
    // 1. Validate the input working image
    if (!working_image.isValid()) {
        spdlog::warn("OperationShadows::execute: Invalid working image provided");
        return false;
    }

    // Skip execution if the operation is disabled
    if (!descriptor.enabled) {
        spdlog::trace("OperationShadows::execute: Operation is disabled, skipping execution");
        return true;
    }

    // 2. Extract the shadow adjustment value parameter from the descriptor
    float shadows_value = descriptor.getParam<float>("value", 0.0f);

    // No-op optimization: Skip processing if the value matches the default
    if (shadows_value == OperationShadows::DEFAULT_SHADOWS_VALUE) {
        spdlog::trace("OperationShadows::execute: Shadow adjustment value is default ({}), skipping operation", OperationShadows::DEFAULT_SHADOWS_VALUE);
        return true;
    }

    // Validate and clamp the shadow adjustment value to the defined operational range
    if (shadows_value < OperationShadows::MIN_SHADOWS_VALUE || shadows_value > OperationShadows::MAX_SHADOWS_VALUE) {
        spdlog::warn("OperationShadows::execute: Shadow adjustment value ({}) is outside the valid range [{}, {}]. Clamping.",
                     shadows_value, OperationShadows::MIN_SHADOWS_VALUE, OperationShadows::MAX_SHADOWS_VALUE);
        shadows_value = std::clamp(shadows_value, OperationShadows::MIN_SHADOWS_VALUE, OperationShadows::MAX_SHADOWS_VALUE);
    }

    spdlog::debug("OperationShadows::execute: Applying shadow adjustment with value={:.2f}", shadows_value);

    // Export the current working image data to a CPU-accessible copy for processing
    auto cpu_copy = working_image.exportToCPUCopy();
    if (!cpu_copy) {
        spdlog::error("OperationShadows::execute: Failed to export working image to CPU copy for processing");
        return false;
    }

    // 3. Execute the Halide-based image processing pipeline
    try {
        spdlog::info("OperationShadows::execute: Creating Halide buffer for processing");

        // Log image dimensions for debugging
        spdlog::info("OperationShadows::execute: Processing image size: {}x{} ({} channels), total elements: {}",
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

        spdlog::info("OperationShadows::execute: Halide buffer created successfully");

        // Appliquer l'ajustement d'ombres en utilisant la fonction utilitaire
        auto shadows_func = applyShadowsAdjustment(input_buf, shadows_value, x, y, c);

        // Schedule the Halide function for optimized parallel execution
        spdlog::info("OperationShadows::execute: Halide function defined with luminance mask logic");
        shadows_func.parallel(y, 8).vectorize(x, 8); // Parallelize over Y-axis, vectorize over X-axis for performance
        spdlog::info("OperationShadows::execute: Parallel and vectorization schedule applied, about to realize");

        // Execute the Halide pipeline and write the result back to the input buffer
        shadows_func.realize(input_buf);
        spdlog::info("OperationShadows::execute: Halide realization completed successfully");

        // Update the working image with the processed CPU copy data
        return working_image.updateFromCPU(*cpu_copy);

    } catch (const std::exception& e) {
        spdlog::critical("OperationShadows::execute: Exception occurred during Halide processing: {}", e.what());
        return false;
    }
}

Halide::Func OperationShadows::appendToFusedPipeline(
    const Halide::Func& input_func,
    const Halide::Var& x,
    const Halide::Var& y,
    const Halide::Var& c,
    const OperationDescriptor& params
    ) const
{
    // Extract the shadow adjustment value parameter from the operation descriptor
    float shadows_value = params.getParam<float>("value", 0.0f);

    // No-op optimization: Return the input function unchanged if the value is at default
    if (shadows_value == OperationShadows::DEFAULT_SHADOWS_VALUE) {
        spdlog::trace("OperationShadows::appendToFusedPipeline: No-op requested, returning input function unchanged");
        return input_func;
    }

    // Validate and clamp the shadow adjustment value to the defined operational range
    if (shadows_value < OperationShadows::MIN_SHADOWS_VALUE || shadows_value > OperationShadows::MAX_SHADOWS_VALUE) {
        spdlog::warn("OperationShadows::appendToFusedPipeline: Clamping adjustment value to valid range [{}, {}]",
                     OperationShadows::MIN_SHADOWS_VALUE, OperationShadows::MAX_SHADOWS_VALUE);
        shadows_value = std::clamp(shadows_value, OperationShadows::MIN_SHADOWS_VALUE, OperationShadows::MAX_SHADOWS_VALUE);
    }

    spdlog::debug("OperationShadows::appendToFusedPipeline: Building fusion fragment with value={:.2f}", shadows_value);

    return applyShadowsAdjustment(input_func, shadows_value, x, y, c);
}

} // namespace CaptureMoment::Core::Operations
