/**
 * @file operation_pipeline_executor_gpu.cpp
 * @brief Implementation of OperationPipelineExecutorGPU.
 * @author CaptureMoment Team
 * @date 2026
 */

#include "pipeline/operations/operation_pipeline_executor_gpu.h"
#include "image_processing/gpu/working_image_gpu_halide.h"
#include "pipeline/helpers/halide_color_space.h"

#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::Pipeline {

void OperationPipelineExecutorGPU::buildOperationChain()
{
    Halide::Var x("x"), y("y"), c("c");

    // 1. Chunky input from Halide::ImageParam (x,y,c) -> (x,y,c)
    Halide::Func rgb_input("rgb_input");
    rgb_input(x, y, c) = m_input(x, y, c);

    // ====================================================================
    // OKLAB DIRECT (No Transpose) + OPERATIONS
    // ====================================================================
    Halide::Func oklab_image { ColorSpace::rgbToOklab(rgb_input, x, y, c) };
    Halide::Func ops_result { applyOperations(oklab_image, x, y, c) };
    Halide::Func final_rgb_output { ColorSpace::oklabToRgb(ops_result, x, y, c) };

    // ====================================================================
    // SCHEDULING & COMPILATION GPU
    // ====================================================================
    final_rgb_output.output_buffer().dim(0).set_stride(4);
    final_rgb_output.output_buffer().dim(2).set_stride(1);

    Halide::Target target { Config::AppConfig::getHalideTarget() };
    applyScheduling(final_rgb_output, x, y, c);

    try {
        final_rgb_output.compile_jit(target);
        m_pipeline = Halide::Pipeline(final_rgb_output);
        m_chain_built = true;
        spdlog::info("[OperationPipelineExecutorGPU::buildOperationChain]: Pipeline compiled (Direct Chunky Oklab).");
    } catch (const Halide::CompileError& e) {
        spdlog::critical("[OperationPipelineExecutorGPU::buildOperationChain]: {}", e.what());
        m_chain_built = false;
    }
}

void OperationPipelineExecutorGPU::applyScheduling(Halide::Func& pipeline, Halide::Var& x, Halide::Var& y, Halide::Var &c) const
{
    Halide::Var xo, yo, xi, yi;
    pipeline.gpu_tile(x, y, xo, yo, xi, yi, 16, 16);
    spdlog::trace("[OperationPipelineExecutorGPU::applyScheduling]: Applying GPU scheduling.");
}

bool OperationPipelineExecutorGPU::execute(ImageProcessing::IWorkingImageHardware& working_image)
{
    auto* gpu_impl{dynamic_cast<ImageProcessing::WorkingImageGPU_Halide*>(&working_image)};

    if (!gpu_impl || !gpu_impl->isHalideBufferValid() || !gpu_impl->isOriginalHalideBufferValid()) {
        spdlog::error("[OperationPipelineExecutorGPU::execute]: Invalid Halide buffer or GPU implementation.");
        return false;
    }
    
    return executeOnHalideBuffer(gpu_impl->getOriginalHalideBuffer(), gpu_impl->getHalideBuffer());
}

} // namespace CaptureMoment::Core::Pipeline
