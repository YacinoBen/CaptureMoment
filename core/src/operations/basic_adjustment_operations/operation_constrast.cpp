/**
 * @file operation_contrast.cpp
 * @brief Implementation of OperationContrast
 * @author CaptureMoment Team
 * @date 2025
 */

#include "operations/basic_adjustment_operations/operation_contrast.h"

#include <spdlog/spdlog.h>
#include <algorithm>

namespace CaptureMoment::Core::Operations {

template<typename InputType>
Halide::Func applyContrastAdjustment(
    const InputType& input,
    float contrast_value,
    const Halide::Var& x,
    const Halide::Var& y,
    const Halide::Var& c
    )
{
    Halide::Func contrast_func("contrast_op");

    // Application de l'ajustement de contraste
    contrast_func(x, y, c) = Halide::select(
        c < 3, // R, G, B
        Halide::clamp(0.5f + (input(x, y, c) - 0.5f) * contrast_value, 0.0f, 1.0f),
        input(x, y, c) // A (Alpha inchangé)
        );

    return contrast_func;
}

bool OperationContrast::execute(ImageProcessing::IWorkingImageHardware& working_image, const OperationDescriptor& descriptor)
{
    // 1. Validate the input working image
    if (!working_image.isValid()) {
        spdlog::warn("OperationContrast::execute: Invalid working image provided");
        return false;
    }

    // Skip execution if the operation is disabled
    if (!descriptor.enabled) {
        spdlog::trace("OperationContrast::execute: Operation is disabled, skipping execution");
        return true;
    }

    // 2. Extract the contrast adjustment value parameter from the descriptor
    float contrast_value = descriptor.getParam<float>("value", 1.0f);

    // No-op optimization: Skip processing if the value matches the default
    if (contrast_value == OperationContrast::DEFAULT_CONTRAST_VALUE) {
        spdlog::trace("OperationContrast::execute: Contrast adjustment value is default ({}), skipping operation", OperationContrast::DEFAULT_CONTRAST_VALUE);
        return true;
    }

    // Validate and clamp the contrast adjustment value to the defined operational range
    if (contrast_value < OperationContrast::MIN_CONTRAST_VALUE || contrast_value > OperationContrast::MAX_CONTRAST_VALUE) {
        spdlog::warn("OperationContrast::execute: Contrast adjustment value ({}) is outside the valid range [{}, {}]. Clamping.",
                     contrast_value, OperationContrast::MIN_CONTRAST_VALUE, OperationContrast::MAX_CONTRAST_VALUE);
        contrast_value = std::clamp(contrast_value, OperationContrast::MIN_CONTRAST_VALUE, OperationContrast::MAX_CONTRAST_VALUE);
    }

    spdlog::debug("OperationContrast::execute: Applying contrast adjustment with value={:.2f}", contrast_value);

    // Export the current working image data to a CPU-accessible copy for processing
    auto cpu_copy = working_image.exportToCPUCopy();
    if (!cpu_copy) {
        spdlog::error("OperationContrast::execute: Failed to export working image to CPU copy for processing");
        return false;
    }

    // 3. Execute the Halide-based image processing pipeline
    try {
        spdlog::info("OperationContrast::execute: Creating Halide buffer for processing");

        // Log image dimensions for debugging
        spdlog::info("OperationContrast::execute: Processing image size: {}x{} ({} channels), total elements: {}",
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

        spdlog::info("OperationContrast::execute: Halide buffer created successfully");

        // Appliquer l'ajustement de contraste en utilisant la fonction utilitaire
        auto contrast_func = applyContrastAdjustment(input_buf, contrast_value, x, y, c);

        // Schedule the Halide function for optimized parallel execution
        spdlog::info("OperationContrast::execute: Halide function defined with contrast logic");
        contrast_func.parallel(y, 8).vectorize(x, 8); // Parallelize over Y-axis, vectorize over X-axis for performance
        spdlog::info("OperationContrast::execute: Parallel and vectorization schedule applied, about to realize");

        // Execute the Halide pipeline and write the result back to the input buffer
        contrast_func.realize(input_buf);
        spdlog::info("OperationContrast::execute: Halide realization completed successfully");

        // Update the working image with the processed CPU copy data
        return working_image.updateFromCPU(*cpu_copy);

    } catch (const std::exception& e) {
        spdlog::critical("OperationContrast::execute: Exception occurred during Halide processing: {}", e.what());
        return false;
    }
}

Halide::Func OperationContrast::appendToFusedPipeline(
    const Halide::Func& input_func,
    const Halide::Var& x,
    const Halide::Var& y,
    const Halide::Var& c,
    const OperationDescriptor& params
    ) const
{
    // Extract the contrast adjustment value parameter from the operation descriptor
    float contrast_value = params.getParam<float>("value", 1.0f);

    // No-op optimization: Return the input function unchanged if the value is at default
    if (contrast_value == OperationContrast::DEFAULT_CONTRAST_VALUE) {
        spdlog::trace("OperationContrast::appendToFusedPipeline: No-op requested, returning input function unchanged");
        return input_func;
    }

    // Validate and clamp the contrast adjustment value to the defined operational range
    if (contrast_value < OperationContrast::MIN_CONTRAST_VALUE || contrast_value > OperationContrast::MAX_CONTRAST_VALUE) {
        spdlog::warn("OperationContrast::appendToFusedPipeline: Clamping adjustment value to valid range [{}, {}]",
                     OperationContrast::MIN_CONTRAST_VALUE, OperationContrast::MAX_CONTRAST_VALUE);
        contrast_value = std::clamp(contrast_value, OperationContrast::MIN_CONTRAST_VALUE, OperationContrast::MAX_CONTRAST_VALUE);
    }

    spdlog::debug("OperationContrast::appendToFusedPipeline: Building fusion fragment with value={:.2f}", contrast_value);

    return applyContrastAdjustment(input_func, contrast_value, x, y, c);
}

} // namespace CaptureMoment::Core::Operations
