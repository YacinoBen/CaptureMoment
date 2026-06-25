/**
 * @file operation_pipeline_executor_gpu.h
 * @brief Declaration of OperationPipelineExecutorGPU (Fused Adjustment Pipeline).
 *
 * @details
 * This class is a concrete implementation of `OperationPipelineExecutor` optimized for GPU execution.
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "pipeline/operations/operation_pipeline_executor.h"

namespace CaptureMoment::Core {

namespace Pipeline {

/**
 * @class OperationPipelineExecutorGPU
 * @brief Concrete executor for fused Halide adjustment pipelines. GPU
 *
 * @details
 * This executor chains multiple image adjustment operations (Brightness, Contrast, etc.)
    * into a single Halide graph. It relies on the `IHalidePipelineExecutor` base class
 * to provide the standardized 4-channel input parameter (`m_input`).
 */
class OperationPipelineExecutorGPU final : public OperationPipelineExecutor {
public:
    /**
     * @brief Destructor.
     */
    ~OperationPipelineExecutorGPU() override = default;

    [[nodiscard]] bool execute(ImageProcessing::IWorkingImageHardware& working_image) override;

private:
    void applyScheduling(Halide::Func& pipeline, Halide::Var& x, Halide::Var& y) const override;
};
} // namespace Pipeline
} // namespace CaptureMoment::Core
