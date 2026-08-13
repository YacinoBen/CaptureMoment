/**
 * @file operation_pipeline_executor_cpu.cpp
 * @brief Implementation of OperationPipelineExecutorCPU.
 * @author CaptureMoment Team
 * @date 2026
 */

#include "pipeline/operations/operation_pipeline_executor_cpu.h"
#include "image_processing/cpu/working_image_cpu_halide.h"
#include "pipeline/helpers/halide_color_space.h"

#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::Pipeline {

void OperationPipelineExecutorCPU::buildOperationChain()
{
    Halide::Var x("x"), y("y"), c("c");

    // 1. Chunky input from Halide::ImageParam (x,y,c) -> (x,y,c)
    Halide::Func chunky_input("chunky_input");
    chunky_input(x, y, c) = m_input(x, y, c);

    // ====================================================================
    // CPU : BOUNDARY TRANSPOSE (Chunky -> Logique Planar)
    // ====================================================================
    Halide::Func planar_input("planar_input");
    planar_input(c, y, x) = chunky_input(x, y, c); //Transpose for planar logic (c,y,x) for math operations

    Halide::Func cpu_ready_input("cpu_ready_input");
    cpu_ready_input(x, y, c) = planar_input(c, y, x); // Transpose back to (x,y,c) for Halide processing

    // ====================================================================
    // CPU : OKLAB COLOR SPACE + OPERATIONS
    // ====================================================================
    Halide::Func oklab_image { ColorSpace::rgbToOklab(cpu_ready_input, x, y, c) };
    Halide::Func ops_result { applyOperations(oklab_image, x, y, c) }; // Apply all operations in the pipeline to the Oklab image
    Halide::Func rgb_planar { ColorSpace::oklabToRgb(ops_result, x, y, c) };

    // ====================================================================
    // CPU : BOUNDARY TRANSPOSE (Logique Planar -> Chunky)
    // ====================================================================
    Halide::Func planar_output("planar_output");
    planar_output(c, y, x) = rgb_planar(x, y, c);

    Halide::Func final_chunky_output("final_chunky_output");
    final_chunky_output(x, y, c) = planar_output(c, y, x);

    // ====================================================================
    // SCHEDULING & COMPILATION CPU
    // ====================================================================
    final_chunky_output.output_buffer().dim(0).set_stride(4);
    final_chunky_output.output_buffer().dim(2).set_stride(1);

    Halide::Target target { Config::AppConfig::getHalideTarget() };
    applyScheduling(final_chunky_output, x, y, c);

    try {
        final_chunky_output.compile_jit(target);
        m_pipeline = Halide::Pipeline(final_chunky_output);
        m_chain_built = true;
        spdlog::info("[OperationPipelineExecutorCPU::buildOperationChain]: Pipeline compiled (Chunky->Planar->Oklab->Ops->Planar->Chunky).");
    } catch (const Halide::CompileError& e) {
        spdlog::critical("[OperationPipelineExecutorCPU::buildOperationChain]: {}", e.what());
        m_chain_built = false;
    }
}

void OperationPipelineExecutorCPU::applyScheduling(Halide::Func& pipeline, Halide::Var& x, Halide::Var& y, Halide::Var& c) const
{
    Halide::Var yo, yi;
    pipeline.bound(c, 0, 4).reorder(c, x, y).split(y, yo, yi, 32).parallel(yo).unroll(c);
    spdlog::trace("[OperationPipelineExecutorCPU::applyScheduling]: Applying CPU scheduling.");
}

bool OperationPipelineExecutorCPU::execute(ImageProcessing::IWorkingImageHardware& working_image)
{
    auto* cpu_impl{dynamic_cast<ImageProcessing::WorkingImageCPU_Halide*>(&working_image)};

    if (!cpu_impl || !cpu_impl->isHalideBufferValid() || !cpu_impl->isOriginalHalideBufferValid()) {
        spdlog::error("[OperationPipelineExecutorCPU::execute]: Invalid Halide buffer or CPU implementation.");
        return false;
    }
    
    return executeOnHalideBuffer(cpu_impl->getOriginalHalideBuffer(), cpu_impl->getHalideBuffer());
}

} // namespace CaptureMoment::Core::Pipeline
