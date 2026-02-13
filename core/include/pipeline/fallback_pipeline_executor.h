/**
 * @file FallbackPipelineExecutor.h
 * @brief Declaration of FallbackPipelineExecutor (Generic Sequential Pipeline).
 *
 * @details
 * This class implements a "Generic Sequential Pipeline" strategy.
 * It serves as a fallback executor when optimized paths (e.g., Halide fusion)
 * are unavailable or unsuitable. It applies operations sequentially on a CPU copy
 * of the image data.
 *
 * **Architecture:**
 * - **Generic**: Works with any `IWorkingImageHardware`.
 * - **Sequential**: Operations are applied one by one.
 * - **Fallback**: Intended for use when fast paths fail.
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "pipeline/interfaces/i_pipeline_executor.h"
#include "operations/operation_descriptor.h"
#include "operations/operation_factory.h"

#include <vector>

namespace CaptureMoment::Core {

namespace Pipeline {

/**
 * @class FallbackPipelineExecutor
 * @brief Concrete implementation for executing a pipeline sequentially as a fallback.
 *
 * @details
 * This executor provides a generic way to apply a series of operations
 * to an image when optimized execution paths are not available.
 * It works by:
 * 1. Exporting the image data to a CPU copy (`ImageRegion`).
 * 2. Applying each enabled operation sequentially to the CPU copy using `IOperationDefaultLogic`.
 * 3. Updating the original image with the processed CPU copy.
 *
 * This is significantly slower than fused execution but ensures compatibility
 * with any `IWorkingImageHardware` implementation.
 */
class FallbackPipelineExecutor final : public IPipelineExecutor
{
public:
    /**
     * @brief Constructs a fallback pipeline executor for a specific list of operations.
     *
     * @details
     * Stores the list of operation descriptors and holds a reference to the factory
     * needed to instantiate the operation logic during execution.
     *
     * @param[in] operations The list of operation descriptors defining the adjustments.
     * @param[in] factory The factory used to instantiate operation instances.
     */
    explicit FallbackPipelineExecutor(
        std::vector<Operations::OperationDescriptor>&& operations,
        const Operations::OperationFactory& factory
        );

    /**
     * @brief Executes the sequential pipeline on a generic working image.
     *
     * @details
     * This method implements the fallback execution logic:
     * 1. Exports the `working_image` to a CPU copy using `exportToCPUCopy()`.
     * 2. Iterates through the stored operations, instantiates each one via the factory,
     *    casts it to `IOperationDefaultLogic`, and applies its `executeOnImageRegion` logic
     *    to the CPU copy.
     * 3. Updates the `working_image` with the processed CPU copy using `updateFromCPU()`.
     *
     * @param[in,out] working_image The hardware-agnostic image to process.
     * @return true if execution succeeded, false otherwise.
     */
    [[nodiscard]] bool execute(ImageProcessing::IWorkingImageHardware& working_image) override;

private:

    /**
     * @brief Stores the list of operations to be applied sequentially.
     */
    std::vector<Operations::OperationDescriptor> m_operations;

    /**
     * @brief Reference to the operation factory for creating logic instances.
     */
    const Operations::OperationFactory& m_factory;
};

} // namespace Pipeline

} // namespace CaptureMoment::Core
