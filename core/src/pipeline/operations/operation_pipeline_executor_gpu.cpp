/**
 * @file operation_pipeline_executor_gpu.cpp
 * @brief Implementation of OperationPipelineExecutorGPU.
 * @author CaptureMoment Team
 * @date 2026
 */

#include "pipeline/operations/operation_pipeline_executor_gpu.h"
#include "image_processing/gpu/working_image_gpu_halide.h"

#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::Pipeline {

void OperationPipelineExecutorGPU::applyScheduling(Halide::Func& pipeline, Halide::Var& x, Halide::Var& y, Halide::Var &c) const
{
    Halide::Var xo, yo, xi, yi;
    pipeline.gpu_tile(x, y, xo, yo, xi, yi, 16, 16);
    spdlog::trace("OperationPipelineExecutorGPU::applyScheduling: Applying GPU scheduling.");
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
