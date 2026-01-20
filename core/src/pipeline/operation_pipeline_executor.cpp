/**
 * @file operation_pipeline_executor.cpp
 * @brief Implementation of OperationPipelineExecutor
 * @author CaptureMoment Team
 * @date 2026
 */

#include "pipeline/operation_pipeline_executor.h"
#include "operations/interfaces/i_operation_fusion_logic.h"
#include "operations/interfaces/i_operation.h"

#include <Halide.h>
#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::Pipeline {

OperationPipelineExecutor::OperationPipelineExecutor(
    const std::vector<Operations::OperationDescriptor>& operations,
    const Operations::OperationFactory& factory
    ) : m_operations(operations), m_factory(factory) {
    // Compile the pipeline once during construction for efficiency
    savePipeline();
}

bool OperationPipelineExecutor::execute(ImageProcessing::IWorkingImageHardware& working_image) const {
    if (!m_saved_pipeline) {
        spdlog::error("OperationPipelineExecutor::execute: Pipeline not compiled or compilation failed.");
        return false;
    }

    // Determine the execution strategy based on the configured backend and concrete image type
    if (m_backend == Common::MemoryType::CPU_RAM) {
        auto* cpu_impl = dynamic_cast<ImageProcessing::WorkingImageCPU_Halide*>(&working_image);
        if (cpu_impl) {
            return executeWithConcreteCPU(*cpu_impl);
        }
    } else if (m_backend == Common::MemoryType::GPU_MEMORY) {
        auto* gpu_impl = dynamic_cast<ImageProcessing::WorkingImageGPU_Halide*>(&working_image);
        if (gpu_impl) {
            return executeWithConcreteGPU(*gpu_impl);
        }
    }

    // If the configured backend doesn't match the concrete implementation,
    // or if the concrete type cannot be determined, fall back to generic implementation
    return executeGeneric(working_image);
}

bool OperationPipelineExecutor::executeWithConcreteCPU(
    ImageProcessing::WorkingImageCPU_Halide& concrete_image
    ) const {
    if (!m_saved_pipeline) {
        spdlog::error("OperationPipelineExecutor::executeWithConcreteCPU: Pipeline not compiled or compilation failed.");
        return false;
    }

    try {
        // Get image dimensions for the execution
        auto [width, height] = concrete_image.getSize();
        size_t channels = concrete_image.getChannels();

        if (width <= 0 || height <= 0 || channels == 0) {
            spdlog::error("OperationPipelineExecutor::executeWithConcreteCPU: Invalid image dimensions ({}x{}x{}).", width, height, channels);
            return false;
        }

        // Export the current working image to CPU for processing with the compiled pipeline
        auto cpu_region = concrete_image.exportToCPUCopy();
        if (!cpu_region) {
            spdlog::error("OperationPipelineExecutor::executeWithConcreteCPU: Failed to export working image to CPU for processing.");
            return false;
        }

        // Create input buffer from the exported CPU copy data
        Halide::Buffer<float> input_buf(
            cpu_region->m_data.data(),
            static_cast<int>(width),
            static_cast<int>(height),
            static_cast<int>(channels)
            );

        // Execute the compiled pipeline
        // The pipeline expects input and output buffers with the same dimensions
        Halide::Realization outputs = m_saved_pipeline->realize({static_cast<int>(width), static_cast<int>(height), static_cast<int>(channels)});

        if (outputs.size() == 0) {
            spdlog::error("OperationPipelineExecutor::executeWithConcreteCPU: Pipeline execution produced no output.");
            return false;
        }

        // Assuming the first output is the processed image
        Halide::Buffer<float> output_buf = outputs[0].as<Halide::Buffer<float>>();

        // Convert the Halide::Buffer output to an ImageRegion using the concrete implementation's own conversion capability
        // This avoids using external converters and leverages the internal conversion logic
        Common::ImageRegion output_region;
        output_region.m_width = output_buf.width();
        output_region.m_height = output_buf.height();
        output_region.m_channels = output_buf.channels();
        output_region.m_format = Common::PixelFormat::RGBA_F32; // Assuming default format

        // Resize the data vector to match the buffer size
        size_t total_elements = static_cast<size_t>(output_buf.width()) * output_buf.height() * output_buf.channels();
        output_region.m_data.resize(total_elements);

        // Copy data from Halide buffer to ImageRegion using internal conversion
        std::memcpy(
            output_region.m_data.data(),
            output_buf.data(),
            total_elements * sizeof(float)
            );

        if (!concrete_image.updateFromCPU(output_region)) {
            spdlog::error("OperationPipelineExecutor::executeWithConcreteCPU: Failed to update working image with pipeline result.");
            return false;
        }

        spdlog::debug("OperationPipelineExecutor::executeWithConcreteCPU: Successfully executed fused pipeline on {} operations.", m_operations.size());
        return true;

    } catch (const Halide::Error& e) {
        spdlog::error("OperationPipelineExecutor::executeWithConcreteCPU: Halide error during execution: {}", e.what());
        return false;
    } catch (const std::exception& e) {
        spdlog::error("OperationPipelineExecutor::executeWithConcreteCPU: Error during execution: {}", e.what());
        return false;
    }
}

bool OperationPipelineExecutor::executeWithConcreteGPU(
    ImageProcessing::WorkingImageGPU_Halide& concrete_image
    ) const {
    if (!m_saved_pipeline) {
        spdlog::error("OperationPipelineExecutor::executeWithConcreteGPU: Pipeline not compiled or compilation failed.");
        return false;
    }

    try {
        // Get image dimensions for the execution
        auto [width, height] = concrete_image.getSize();
        size_t channels = concrete_image.getChannels();

        if (width <= 0 || height <= 0 || channels == 0) {
            spdlog::error("OperationPipelineExecutor::executeWithConcreteGPU: Invalid image dimensions ({}x{}x{}).", width, height, channels);
            return false;
        }

        // Export the current working image to CPU for processing with the compiled pipeline
        auto cpu_region = concrete_image.exportToCPUCopy();
        if (!cpu_region) {
            spdlog::error("OperationPipelineExecutor::executeWithConcreteGPU: Failed to export working image to CPU for processing.");
            return false;
        }

        // Create input buffer from the exported CPU copy data
        Halide::Buffer<float> input_buf(
            cpu_region->m_data.data(),
            static_cast<int>(width),
            static_cast<int>(height),
            static_cast<int>(channels)
            );

        // Execute the compiled pipeline
        // The pipeline expects input and output buffers with the same dimensions
        Halide::Realization outputs = m_saved_pipeline->realize({static_cast<int>(width), static_cast<int>(height), static_cast<int>(channels)});

        if (outputs.size() == 0) {
            spdlog::error("OperationPipelineExecutor::executeWithConcreteGPU: Pipeline execution produced no output.");
            return false;
        }

        // Assuming the first output is the processed image
        Halide::Buffer<float> output_buf = outputs[0].as<Halide::Buffer<float>>();

        // Convert the Halide::Buffer output to an ImageRegion using the concrete implementation's own conversion capability
        // This avoids using external converters and leverages the internal conversion logic
        Common::ImageRegion output_region;
        output_region.m_width = output_buf.width();
        output_region.m_height = output_buf.height();
        output_region.m_channels = output_buf.channels();
        output_region.m_format = Common::PixelFormat::RGBA_F32; // Assuming default format

        // Resize the data vector to match the buffer size
        size_t total_elements = static_cast<size_t>(output_buf.width()) * output_buf.height() * output_buf.channels();
        output_region.m_data.resize(total_elements);

        // Copy data from Halide buffer to ImageRegion using internal conversion
        std::memcpy(
            output_region.m_data.data(),
            output_buf.data(),
            total_elements * sizeof(float)
            );

        if (!concrete_image.updateFromCPU(output_region)) {
            spdlog::error("OperationPipelineExecutor::executeWithConcreteGPU: Failed to update working image with pipeline result.");
            return false;
        }

        spdlog::debug("OperationPipelineExecutor::executeWithConcreteGPU: Successfully executed fused pipeline on {} operations.", m_operations.size());
        return true;

    } catch (const Halide::Error& e) {
        spdlog::error("OperationPipelineExecutor::executeWithConcreteGPU: Halide error during execution: {}", e.what());
        return false;
    } catch (const std::exception& e) {
        spdlog::error("OperationPipelineExecutor::executeWithConcreteGPU: Error during execution: {}", e.what());
        return false;
    }
}

bool OperationPipelineExecutor::executeGeneric(
    ImageProcessing::IWorkingImageHardware& working_image
    ) const {
    if (!m_saved_pipeline) {
        spdlog::error("OperationPipelineExecutor::executeGeneric: Pipeline not compiled or compilation failed.");
        return false;
    }

    try {
        // Get image dimensions for the execution
        auto [width, height] = working_image.getSize();
        size_t channels = working_image.getChannels();

        if (width <= 0 || height <= 0 || channels == 0) {
            spdlog::error("OperationPipelineExecutor::executeGeneric: Invalid image dimensions ({}x{}x{}).", width, height, channels);
            return false;
        }

        // Export the current working image to CPU for processing with the compiled pipeline
        auto cpu_region = working_image.exportToCPUCopy();
        if (!cpu_region) {
            spdlog::error("OperationPipelineExecutor::executeGeneric: Failed to export working image to CPU for processing.");
            return false;
        }

        // Create input buffer from the exported CPU copy data
        Halide::Buffer<float> input_buf(
            cpu_region->m_data.data(),
            static_cast<int>(width),
            static_cast<int>(height),
            static_cast<int>(channels)
            );

        // Execute the compiled pipeline
        // The pipeline expects input and output buffers with the same dimensions
        Halide::Realization outputs = m_saved_pipeline->realize({static_cast<int>(width), static_cast<int>(height), static_cast<int>(channels)});

        if (outputs.size() == 0) {
            spdlog::error("OperationPipelineExecutor::executeGeneric: Pipeline execution produced no output.");
            return false;
        }

        // Assuming the first output is the processed image
        Halide::Buffer<float> output_buf = outputs[0].as<Halide::Buffer<float>>();

        // Convert the Halide::Buffer output to an ImageRegion manually
        Common::ImageRegion output_region;
        output_region.m_width = output_buf.width();
        output_region.m_height = output_buf.height();
        output_region.m_channels = output_buf.channels();
        output_region.m_format = Common::PixelFormat::RGBA_F32; // Assuming default format

        // Resize the data vector to match the buffer size
        size_t total_elements = static_cast<size_t>(output_buf.width()) * output_buf.height() * output_buf.channels();
        output_region.m_data.resize(total_elements);

        // Copy data from Halide buffer to ImageRegion
        std::memcpy(
            output_region.m_data.data(),
            output_buf.data(),
            total_elements * sizeof(float)
            );

        if (!working_image.updateFromCPU(output_region)) {
            spdlog::error("OperationPipelineExecutor::executeGeneric: Failed to update working image with pipeline result.");
            return false;
        }

        spdlog::debug("OperationPipelineExecutor::executeGeneric: Successfully executed fused pipeline on {} operations.", m_operations.size());
        return true;

    } catch (const Halide::Error& e) {
        spdlog::error("OperationPipelineExecutor::executeGeneric: Halide error during execution: {}", e.what());
        return false;
    } catch (const std::exception& e) {
        spdlog::error("OperationPipelineExecutor::executeGeneric: Error during execution: {}", e.what());
        return false;
    }
}

void OperationPipelineExecutor::savePipeline() const {
    if (m_operations.empty()) {
        spdlog::error("OperationPipelineExecutor::savePipeline: Cannot compile empty pipeline.");
        return;
    }

    try {
        // Create Halide variables for the coordinate system shared across all operations in the pipeline
        Halide::Var x, y, c;

        // Create the input function for the pipeline (placeholder for the image data)
        Halide::Func input_func("input_image");

        // Start with the input function and chain operations together
        Halide::Func current_func = input_func;

        for (const auto& desc : m_operations) {
            if (!desc.enabled) {
                continue;
            }

            // Create the operation instance to get its fusion logic
            auto op_impl = m_factory.create(desc);
            if (!op_impl) {
                spdlog::error("OperationPipelineExecutor::savePipeline: Cannot create operation '{}'. Skipping.", desc.name);
                continue;
            }

            // Cast to the fusion logic interface
            auto fusion_logic = dynamic_cast<const Operations::IOperationFusionLogic*>(op_impl.get());
            if (!fusion_logic) {
                spdlog::warn("OperationPipelineExecutor::savePipeline: Operation '{}' does not support fusion. Skipping.", desc.name);
                continue;
            }

            // Append this operation's logic to the pipeline
            current_func = fusion_logic->appendToFusedPipeline(current_func, x, y, c, desc);
        }

        // Apply scheduling based on the configured backend (CPU or GPU)
        if (m_backend == Common::MemoryType::GPU_MEMORY) {
            Halide::Var xo, yo, xi, yi;
            current_func.gpu_tile(x, y, xo, yo, xi, yi, 16, 16);
        } else {
            current_func.vectorize(x, 8).parallel(y);
        }

        // Compile the final combined function into a Halide::Pipeline
        m_saved_pipeline = std::make_unique<Halide::Pipeline>(current_func);

        spdlog::info("OperationPipelineExecutor::savePipeline: Successfully compiled pipeline for {} operations.", m_operations.size());

    } catch (const std::exception& e) {
        spdlog::error("OperationPipelineExecutor::savePipeline: Error during pipeline compilation: {}", e.what());
        m_saved_pipeline.reset(); // Ensure the pipeline pointer is null on failure
    }
}

} // namespace CaptureMoment::Core::Pipeline
