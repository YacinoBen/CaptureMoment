/**
 * @file operation_pipeline_executor.cpp
 * @brief Implementation of OperationPipelineExecutor
 * @author CaptureMoment Team
 * @date 2026
 */

#include "pipeline/interfaces/operation_pipeline_executor.h"
#include "operations/i_operation_fusion_logic.h"
#include "config/app_config.h"
#include "image_processing/cpu/working_image_cpu_halide.h"
#include "image_processing/gpu/working_image_gpu_halide.h"
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
    // Try to cast to WorkingImageCPU_Halide for direct access to internal conversion
    auto* cpu_impl = dynamic_cast<ImageProcessing::WorkingImageCPU_Halide*>(&working_image);
    if (cpu_impl) {
        return executeWithConcreteImplementation(*cpu_impl);
    }

    // Try to cast to WorkingImageGPU_Halide for direct access to internal conversion
    auto* gpu_impl = dynamic_cast<ImageProcessing::WorkingImageGPU_Halide*>(&working_image);
    if (gpu_impl) {
        // For GPU implementation, we still need to handle the conversion appropriately
        // This might involve transferring data back to CPU temporarily
        return executeGeneric(working_image);
    }

    // Fallback to generic implementation if concrete type is unknown
    return executeGeneric(working_image);
}

bool OperationPipelineExecutor::executeWithConcreteImplementation(
    ImageProcessing::WorkingImageCPU_Halide& concrete_image
) const {
    if (!m_saved_pipeline) {
        spdlog::error("OperationPipelineExecutor::executeWithConcreteImplementation: Pipeline not compiled or compilation failed.");
        return false;
    }

    try {
        // Get image dimensions for the execution
        auto [width, height] = concrete_image.getSize();
        size_t channels = concrete_image.getChannels();

        if (width <= 0 || height <= 0 || channels == 0) {
            spdlog::error("OperationPipelineExecutor::executeWithConcreteImplementation: Invalid image dimensions ({}x{}x{}).", width, height, channels);
            return false;
        }

        // Export the current working image to CPU for processing with the compiled pipeline
        // This uses the internal Halide buffer directly to avoid unnecessary copying
        auto cpu_region = concrete_image.exportToCPUCopy();
        if (!cpu_region) {
            spdlog::error("OperationPipelineExecutor::executeWithConcreteImplementation: Failed to export working image to CPU for processing.");
            return false;
        }

        // Create input buffer from the exported CPU copy data
        Halide::Buffer<float> input_buf(
            cpu_region->m_data.data(),
            width,
            height,
            channels
        );

        // Execute the compiled pipeline
        // The pipeline expects input and output buffers with the same dimensions
        std::vector<Halide::Buffer<>> outputs = m_saved_pipeline->realize({width, height, channels});
        
        if (outputs.empty()) {
            spdlog::error("OperationPipelineExecutor::executeWithConcreteImplementation: Pipeline execution produced no output.");
            return false;
        }

        // Assuming the first output is the processed image
        Halide::Buffer<float> output_buf = outputs[0];

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
            spdlog::error("OperationPipelineExecutor::executeWithConcreteImplementation: Failed to update working image with pipeline result.");
            return false;
        }

        spdlog::debug("OperationPipelineExecutor::executeWithConcreteImplementation: Successfully executed fused pipeline on {} operations.", m_operations.size());
        return true;

    } catch (const Halide::Error& e) {
        spdlog::error("OperationPipelineExecutor::executeWithConcreteImplementation: Halide error during execution: {}", e.what());
        return false;
    } catch (const std::exception& e) {
        spdlog::error("OperationPipelineExecutor::executeWithConcreteImplementation: Error during execution: {}", e.what());
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
            width,
            height,
            channels
        );

        // Execute the compiled pipeline
        // The pipeline expects input and output buffers with the same dimensions
        std::vector<Halide::Buffer<>> outputs = m_saved_pipeline->realize({width, height, channels});
        
        if (outputs.empty()) {
            spdlog::error("OperationPipelineExecutor::executeGeneric: Pipeline execution produced no output.");
            return false;
        }

        // Assuming the first output is the processed image
        Halide::Buffer<float> output_buf = outputs[0];

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
                spdlog::error("OperationPipelineExecutor::compilePipeline: Cannot create operation '{}'. Skipping.", desc.name);
                continue;
            }

            // Cast to the fusion logic interface
            auto fusion_logic = dynamic_cast<const Operations::IOperationFusionLogic*>(op_impl.get());
            if (!fusion_logic) {
                spdlog::warn("OperationPipelineExecutor::compilePipeline: Operation '{}' does not support fusion. Skipping.", desc.name);
                continue;
            }

            // Append this operation's logic to the pipeline
            current_func = fusion_logic->appendToFusedPipeline(current_func, x, y, c, desc);
        }

        // Apply scheduling based on the configured backend (CPU or GPU)
        auto backend = Config::AppConfig::instance().getProcessingBackend();
        if (backend == Common::MemoryType::GPU_MEMORY) {
            Halide::Var xo, yo, xi, yi;
            current_func.gpu_tile(x, y, xo, yo, xi, yi, 16, 16);
        } else {
            current_func.vectorize(x, 8).parallel(y);
        }

        // Compile the final combined function into a Halide::Pipeline
        m_saved_pipeline = std::make_unique<Halide::Pipeline>(current_func);

        spdlog::info("OperationPipelineExecutor::compilePipeline: Successfully compiled pipeline for {} operations.", m_operations.size());

    } catch (const std::exception& e) {
        spdlog::error("OperationPipelineExecutor::compilePipeline: Error during pipeline compilation: {}", e.what());
        m_saved_pipeline.reset(); // Ensure the pipeline pointer is null on failure
    }
}

} // namespace CaptureMoment::Core::Pipeline
