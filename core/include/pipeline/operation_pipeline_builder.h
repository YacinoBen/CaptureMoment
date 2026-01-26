/**
 * @file operation_pipeline_builder.h
 * @brief Declaration of `OperationPipelineBuilder` (Optimized).
 *
 * @details
 * This class is responsible for building a fused Halide pipeline for adjustment operations.
 * @details
 * It acts as a Factory/Builder. It takes a list of operation descriptors (e.g., Brightness, Contrast)
 * and constructs a single, optimized Halide pipeline. The result is an `IPipelineExecutor`
 * object, which can then be used to execute the pipeline.
 *
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
 * @class OperationPipelineBuilder
 * @brief Factory for constructing fused adjustment pipelines.
 *
 * @details
 * This class acts as a Factory/Builder for fused adjustment pipelines.
 * It isolates the "Construction Phase" (building/executing the pipeline) from the
 * "Execution Phase" (running it on an image).
 *
 * The heavy lifting (constructing the JIT code) happens in `OperationPipelineExecutor`.
 * The result is cached for reuse.
 *
 * @note
 * This builder is specifically for "Adjustments". If you add support for "Filters" or "Effects",
 * you would create a new `FilterPipelineBuilder`.
 */
class OperationPipelineBuilder {
public:
    /**
     * @brief Builds a fused pipeline for a given list of operations.
     *
     * @details
     * This static method creates an `OperationPipelineExecutor` instance.
     * The executor will compile the fused pipeline once and cache it.
     *
     * @param[in] operations A vector of `OperationDescriptor` objects defining the sequence.
     * @param[in] factory The `OperationFactory` instance.
     * @return A unique pointer to an `IPipelineExecutor`. Returns nullptr if empty list or build failure.
     */
    [[nodiscard]] static std::unique_ptr<IPipelineExecutor> build(
        const std::vector<Operations::OperationDescriptor>& operations,
        const Operations::OperationFactory& factory
        );

};

} // namespace Pipeline

} // namespace CaptureMoment::Core
