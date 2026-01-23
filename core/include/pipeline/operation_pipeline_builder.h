/**
 * @file operation_pipeline_builder.h
 * @brief Declaration of OperationPipelineBuilder for pipeline fusion of adjustment operations.
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "pipeline/interfaces/i_pipeline_executor.h"
#include "operations/operation_descriptor.h"
#include "operations/operation_factory.h"

#include <vector>
#include <memory>

namespace CaptureMoment::Core {

namespace Pipeline {

/**
 * @brief Class responsible for building a fused Halide pipeline for adjustment operations.
 *
 * OperationPipelineBuilder analyzes a sequence of OperationDescriptors related to
 * image adjustments (e.g., Brightness, Contrast) and constructs a single, optimized
 * Halide pipeline that combines all the specified operations. The result is an
 * OperationPipelineExecutor object, which can then be used to execute the pipeline
 * on an IWorkingImageHardware.
 *
 * This class embodies the core of the adjustment pipeline fusion strategy, centralizing
 * the logic for combining multiple adjustment operations into a single computational pass.
 */
class OperationPipelineBuilder {
public:
    /**
     * @brief Builds a fused Halide pipeline for the given adjustment operations.
     *
     * This static method takes a list of adjustment operations, analyzes them, and constructs
     * a combined Halide pipeline. It schedules it for the appropriate backend
     * (CPU/GPU based on IWorkingImageHardware's type or AppConfig).
     *
     * @param[in] operations A vector of OperationDescriptor objects defining
     *                       the sequence of adjustment operations to apply and fuse.
     * @param[in] factory The OperationFactory instance used to potentially
     *                    retrieve operation-specific pipeline fragments or metadata
     *                    required for fusion (though fusion logic is internal).
     * @return A unique pointer to an IPipelineExecutor object, which encapsulates
     *         the compiled pipeline and provides an execute method.
     *         Returns nullptr if the pipeline construction fails.
     */
    [[nodiscard]] static std::unique_ptr<IPipelineExecutor> build(
        const std::vector<Operations::OperationDescriptor>& operations,
        const Operations::OperationFactory& factory
    );
};

} // namespace Pipeline

} // namespace CaptureMoment::Core
