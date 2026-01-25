/**
 * @file operation_pipeline_builder.h
 * @brief Declaration of OperationPipelineBuilder for creating fused adjustment pipelines.
 *
 * @details
 * This class is a Factory/Builder. It takes a list of adjustment
 * operations (Brightness, Contrast, etc.) and constructs a single, optimized
 * Halide pipeline that combines them.
 *
 * The heavy lifting (building the computation graph) happens in the constructor
 * of the resulting `OperationPipelineExecutor`. This executor is then reused for
 * every frame/interaction, providing maximum performance.
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "pipeline/interfaces/i_pipeline_executor.h"
#include "pipeline/interfaces/i_halide_pipeline_executor.h"
#include "operations/operation_descriptor.h"
#include "operations/operation_factory.h"

#include <vector>
#include <memory>

namespace CaptureMoment::Core {

namespace Pipeline {

/**
 * @class OperationPipelineBuilder
 * @brief Factory for building `OperationPipelineExecutor` objects.
 *
 * @details
 * This class orchestrates the construction of the pipeline:
 * 1. Iterating through operation descriptors.
 * 2. Retrieving their fusion logic (via `IOperationFusionLogic`).
 * 3. Constructing a single Halide::Func graph (Operator Fusion).
 * 4. Creating the `OperationPipelineExecutor` instance which stores the compiled graph.
 *
 * The result is an `OperationPipelineExecutor` object ready for fast execution.
 */
class OperationPipelineBuilder {
public:
    /**
     * @brief Builds a fused pipeline executor for a given list of operations.
     *
     * @details
     * This static method creates an `OperationPipelineExecutor` instance.
     * The executor will compile the fused Halide pipeline once (in its constructor)
     * and reuse it for every `execute` call. This is crucial for performance
     * in interactive applications where parameters might change but the set of
     * operations usually remains stable.
     *
     * @param[in] operations A vector of `OperationDescriptor` objects defining the sequence.
     * @param[in] factory The factory used to instantiate operations for their fusion logic.
     * @return A unique pointer to the `OperationPipelineExecutor`. Returns nullptr on failure.
     */
    [[nodiscard]] static std::unique_ptr<IPipelineExecutor> build(
        const std::vector<Operations::OperationDescriptor>& operations,
        const Operations::OperationFactory& factory
        );
};

} // namespace Pipeline

} // namespace CaptureMoment::Core
