/**
 * @file operation_pipeline_executor_cpu.cpp
 * @brief Implementation of OperationPipelineExecutorCPU.
 * @author CaptureMoment Team
 * @date 2026
 */

#include "pipeline/operations/operation_pipeline_executor_cpu.h"
#include "image_processing/cpu/working_image_cpu_halide.h"

#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::Pipeline {

void OperationPipelineExecutorCPU::applyScheduling(Halide::Func& pipeline, Halide::Var& x, Halide::Var& y) const
{
    Halide::Var var_x, var_y;
    pipeline.split(y, var_y, var_x, 8).parallel(var_y).vectorize(var_x, 8);
    spdlog::trace("OperationPipelineExecutorCPU::applyScheduling: Applying CPU scheduling.");
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
