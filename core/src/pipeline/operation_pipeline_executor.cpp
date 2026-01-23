/**
 * @file operation_pipeline_executor.cpp
 * @brief Implementation of OperationPipelineExecutor (OPTIMIZED)
 * @author CaptureMoment Team
 * @date 2026
 */

#include "pipeline/operation_pipeline_executor.h"
#include "operations/interfaces/i_operation.h"
#include "operations/interfaces/i_operation_fusion_logic.h"
#include "image_processing/cpu/working_image_cpu_halide.h"
#include "image_processing/gpu/working_image_gpu_halide.h"
#include <Halide.h>
#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::Pipeline {

OperationPipelineExecutor::OperationPipelineExecutor(
    const std::vector<Operations::OperationDescriptor>& operations,
    const Operations::OperationFactory& factory
    ) : m_operations(operations), m_factory(factory)
{
    // Pipeline is compiled at execution time, not construction time
    // (because we need the actual image dimensions)
}

bool OperationPipelineExecutor::execute(ImageProcessing::IWorkingImageHardware& working_image) const
{

    // Determine concrete type and execute directly
    if (auto* cpu_impl = dynamic_cast<ImageProcessing::WorkingImageCPU_Halide*>(&working_image)) {
        return executeWithConcreteCPU(*cpu_impl);
    }
    else if (auto* gpu_impl = dynamic_cast<ImageProcessing::WorkingImageGPU_Halide*>(&working_image)) {
        return executeWithConcreteGPU(*gpu_impl);
    }

    spdlog::error("OperationPipelineExecutor::execute: Unsupported working image type.");
    return false;
}

bool OperationPipelineExecutor::executeWithConcreteCPU(ImageProcessing::WorkingImageCPU_Halide& concrete_image) const
{
    try {
        // Verify that image is valid
        if (!concrete_image.isValid()) {
            spdlog::error("OperationPipelineExecutor::executeWithConcreteCPU: Working image is invalid.");
            return false;
        }

        // Get dimensions
        auto [width, height] = concrete_image.getSize();
        size_t channels = concrete_image.getChannels();

        if (width <= 0 || height <= 0 || channels == 0) {
            spdlog::error("OperationPipelineExecutor::executeWithConcreteCPU: Invalid image dimensions ({}x{}x{}).",
                          width, height, channels);
            return false;
        }

        spdlog::debug("OperationPipelineExecutor::executeWithConcreteCPU: Starting pipeline execution on {}x{}x{} image.",
                      width, height, channels);

        // Get the Halide buffer that points to m_data
        // This buffer was initialized by updateFromCPU() in WorkingImageCPU_Halide
        Halide::Buffer<float> working_buffer = concrete_image.getHalideBuffer();

        if (!working_buffer.defined()) {
            spdlog::error("OperationPipelineExecutor::executeWithConcreteCPU: Halide buffer is not defined.");
            return false;
        }

        // Verify that dimensions match
        if (working_buffer.width() != static_cast<int>(width) ||
            working_buffer.height() != static_cast<int>(height) ||
            working_buffer.channels() != static_cast<int>(channels)) {
            spdlog::error("OperationPipelineExecutor::executeWithConcreteCPU: Buffer dimensions mismatch. "
                          "Buffer: {}x{}x{}, Expected: {}x{}x{}",
                          working_buffer.width(), working_buffer.height(), working_buffer.channels(),
                          width, height, channels);
            return false;
        }

        // Execute the pipeline directly on the buffer
        // The pipeline realizes directly into this buffer
        // Zero intermediate copy!
        if (!executePipelineOnBuffer(working_buffer, width, height, channels)) {
            spdlog::error("OperationPipelineExecutor::executeWithConcreteCPU: Pipeline execution failed.");
            return false;
        }

        spdlog::debug("OperationPipelineExecutor::executeWithConcreteCPU: Successfully executed fused pipeline on {} operations.",
                      m_operations.size());
        return true;

    } catch (const Halide::Error& e) {
        spdlog::error("OperationPipelineExecutor::executeWithConcreteCPU: Halide error: {}", e.what());
        return false;
    } catch (const std::exception& e) {
        spdlog::error("OperationPipelineExecutor::executeWithConcreteCPU: Exception: {}", e.what());
        return false;
    }
}

bool OperationPipelineExecutor::executeWithConcreteGPU(ImageProcessing::WorkingImageGPU_Halide& concrete_image) const
{
    try {
        // Verify that image is valid
        if (!concrete_image.isValid()) {
            spdlog::error("OperationPipelineExecutor::executeWithConcreteGPU: Working image is invalid.");
            return false;
        }

        // Get dimensions
        auto [width, height] = concrete_image.getSize();
        size_t channels = concrete_image.getChannels();

        if (width <= 0 || height <= 0 || channels == 0) {
            spdlog::error("OperationPipelineExecutor::executeWithConcreteGPU: Invalid image dimensions ({}x{}x{}).",
                          width, height, channels);
            return false;
        }

        spdlog::debug("OperationPipelineExecutor::executeWithConcreteGPU: Starting pipeline execution on {}x{}x{} image (GPU).",
                      width, height, channels);

        // Same approach for GPU
        Halide::Buffer<float> working_buffer = concrete_image.getHalideBuffer();

        if (!working_buffer.defined()) {
            spdlog::error("OperationPipelineExecutor::executeWithConcreteGPU: Halide buffer is not defined.");
            return false;
        }

        // Verify dimensions
        if (working_buffer.width() != static_cast<int>(width) ||
            working_buffer.height() != static_cast<int>(height) ||
            working_buffer.channels() != static_cast<int>(channels)) {
            spdlog::error("OperationPipelineExecutor::executeWithConcreteGPU: Buffer dimensions mismatch. "
                          "Buffer: {}x{}x{}, Expected: {}x{}x{}",
                          working_buffer.width(), working_buffer.height(), working_buffer.channels(),
                          width, height, channels);
            return false;
        }

        // Execute the pipeline
        if (!executePipelineOnBuffer(working_buffer, width, height, channels)) {
            spdlog::error("OperationPipelineExecutor::executeWithConcreteGPU: Pipeline execution failed.");
            return false;
        }

        spdlog::debug("OperationPipelineExecutor::executeWithConcreteGPU: Successfully executed fused pipeline on {} operations.",
                      m_operations.size());
        return true;

    } catch (const Halide::Error& e) {
        spdlog::error("OperationPipelineExecutor::executeWithConcreteGPU: Halide error: {}", e.what());
        return false;
    } catch (const std::exception& e) {
        spdlog::error("OperationPipelineExecutor::executeWithConcreteGPU: Exception: {}", e.what());
        return false;
    }
}

bool OperationPipelineExecutor::executePipelineOnBuffer(
    Halide::Buffer<float>& buffer,
    size_t width,
    size_t height,
    size_t channels
    ) const
{
    if (m_operations.empty()) {
        spdlog::debug("OperationPipelineExecutor::executePipelineOnBuffer: Empty operation list, nothing to execute.");
        return true;
    }

    try {
        spdlog::trace("OperationPipelineExecutor::executePipelineOnBuffer: Building pipeline for {} operations.",
                      m_operations.size());

        // Create Halide variables
        Halide::Var x, y, c;

        // Create input function that references the existing buffer
        Halide::Func input_func("input_image");
        input_func(x, y, c) = buffer(x, y, c);

        // Chain operations
        Halide::Func current_func = input_func;
        int operation_count = 0;

        for (const auto& desc : m_operations) {
            if (!desc.enabled) {
                spdlog::trace("OperationPipelineExecutor::executePipelineOnBuffer: Skipping disabled operation '{}'",
                              desc.name);
                continue;
            }

            // Create operation instance
            auto op_impl = m_factory.create(desc);
            if (!op_impl) {
                spdlog::warn("OperationPipelineExecutor::executePipelineOnBuffer: Cannot create operation '{}'. Skipping.",
                             desc.name);
                continue;
            }

            // Verify that operation supports fusion
            auto fusion_logic = dynamic_cast<const Operations::IOperationFusionLogic*>(op_impl.get());
            if (!fusion_logic) {
                spdlog::warn("OperationPipelineExecutor::executePipelineOnBuffer: Operation '{}' does not support fusion. Skipping.",
                             desc.name);
                continue;
            }

            // Add this operation to the pipeline
            current_func = fusion_logic->appendToFusedPipeline(current_func, x, y, c, desc);
            operation_count++;

            spdlog::trace("OperationPipelineExecutor::executePipelineOnBuffer: Added operation '{}' to pipeline",
                          desc.name);
        }

        if (operation_count == 0) {
            spdlog::debug("OperationPipelineExecutor::executePipelineOnBuffer: No valid enabled operations to execute.");
            return true;
        }

        spdlog::debug("OperationPipelineExecutor::executePipelineOnBuffer: Pipeline built with {} enabled operations.",
                      operation_count);

        // Apply scheduling
        if (m_backend == Common::MemoryType::GPU_MEMORY) {
            spdlog::trace("OperationPipelineExecutor::executePipelineOnBuffer: Applying GPU scheduling");
            Halide::Var xo, yo, xi, yi;
            current_func.gpu_tile(x, y, xo, yo, xi, yi, 16, 16);
        } else {
            spdlog::trace("OperationPipelineExecutor::executePipelineOnBuffer: Applying CPU scheduling");
            current_func.vectorize(x, 8).parallel(y);
        }

        // Execute the pipeline directly into the provided buffer
        // This writes the results directly into the buffer that points to m_data
        current_func.realize(buffer);

        spdlog::debug("OperationPipelineExecutor::executePipelineOnBuffer: Pipeline executed successfully. "
                      "Results written in-place to buffer ({}x{}x{}).",
                      width, height, channels);
        return true;

    } catch (const Halide::Error& e) {
        spdlog::error("OperationPipelineExecutor::executePipelineOnBuffer: Halide error: {}", e.what());
        return false;
    } catch (const std::exception& e) {
        spdlog::error("OperationPipelineExecutor::executePipelineOnBuffer: Exception: {}", e.what());
        return false;
    }
}

} // namespace CaptureMoment::Core::Pipeline
