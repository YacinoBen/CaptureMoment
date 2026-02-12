/**
 * @file operation_pipeline_executor.h
 * @brief Declaration of OperationPipelineExecutor (Fused Adjustment Pipeline).
 *
 * @details
 * This class is a concrete implementation of `IPipelineExecutor` (and
 * implicitly `IHalidePipelineExecutor`).
 *
 * It implements the "Fused Pipeline" strategy for image adjustments.
 * Instead of executing:
 * 1. `Brightnes -> Image`
 * 2. `Contrast -> Result`
 * 3. `Black -> Result` (3 memory reads/writes)
 *
 * It compiles a single function: `output = black(contrast(brightness(input)))`.
 * This eliminates intermediate buffer allocations/deallocations (Zero-Copy).
 *
 * **Architecture:**
 * - **Caching**: The logic to chain operations is built once.
 *   The final Pipeline compilation happens in executeOnHalideBuffer.
 * - **Backend Support**: Works with both `WorkingImageCPU_Halide` and `WorkingImageGPU_Halide`.
 *   It retrieves the appropriate `Halide::Target` from `AppConfig`.
 * - **Fast Path**: Uses `executeOnHalideBuffer()` when the underlying image
 *   supports it (e.g., `WorkingImageHalide`), skipping generic wrappers.
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "pipeline/interfaces/i_pipeline_executor.h"
#include "pipeline/interfaces/i_halide_pipeline_executor.h"
#include "operations/operation_descriptor.h"
#include "operations/operation_factory.h"
#include "common/types/memory_type.h"

#include <Halide.h>
#include <vector>
#include <functional>

namespace CaptureMoment::Core {

namespace Pipeline {

/**
 * @class OperationPipelineExecutor
 * @brief Concrete implementation for executing fused adjustment pipelines.
 *
 * @details
 * This class encapsulates the heavy lifting (building the Halide function graph)
 * and the cached execution logic.
 * The `execute` method provides a generic entry point, while `executeOnHalideBuffer`
 * offers a high-performance path when the raw buffer is available.
 * This executor is optimized for fused Halide pipelines and does not handle
 * generic fallback execution. Use `FallbackPipelineExecutor` for that.
 */
class OperationPipelineExecutor final : public IPipelineExecutor, public IHalidePipelineExecutor {
public:
    /**
     * @brief Constructs a fused pipeline executor for a specific list of operations.
     *
     * @details
     * The constructor builds the logic to chain operations.
     * The actual Pipeline compilation and scheduling happen in executeOnHalideBuffer.
     *
     * @param[in] operations The list of operation descriptors defining the adjustments.
     * @param[in] factory The factory used to instantiate operation instances.
     */
    OperationPipelineExecutor(
        std::vector<Operations::OperationDescriptor>&& operations,
        const Operations::OperationFactory& factory
        );

    OperationPipelineExecutor(
        const std::vector<Operations::OperationDescriptor>& operations,
        const Operations::OperationFactory& factory
        );


    /**
     * @brief Executes the fused pipeline on a generic working image.
     *
     * @details
     * This method determines the concrete type of the working image (CPU or GPU)
     * based on the configured backend in AppConfig, and dispatches to the
     * appropriate specialized method (`executeWithConcreteHalide`).
     * It serves as the main entry point for the generic `IPipelineExecutor` interface.
     * If the working image type does not match the configured backend,
     * execution will fail.
     *
     * @param[in,out] working_image The hardware-agnostic image to process.
     * @return true if execution succeeded, false otherwise (e.g., incompatible image type).
     */
    [[nodiscard]] bool execute(ImageProcessing::IWorkingImageHardware& working_image) override;

    /**
     * @brief Executes the fused pipeline directly on a raw Halide buffer (Fast Path).
     *
     * @details
     * This method implements `IHalidePipelineExecutor`.
     * It runs the cached compiled pipeline on the provided buffer.
     * This is the most efficient way to process an image when the underlying
     * `WorkingImageHalide` (CPU/GPU) object is used.
     *
     * @param[in,out] buffer The `Halide::Buffer<float>` pointing to the image data.
     * @return true if pipeline executed successfully, false otherwise.
     */
    [[nodiscard]] virtual bool executeOnHalideBuffer(Halide::Buffer<float>& buffer) const override;

private:
    // ============================================================
    // Members
    // ============================================================

    /**
     * @brief Stores the list of operations to be fused.
     * Used to detect if a rebuild is needed if this executor is reused later.
     */
    std::vector<Operations::OperationDescriptor> m_operations;

    /**
     * @brief Reference to the operation factory for creating logic instances.
     */
    const Operations::OperationFactory& m_factory;

    /**
     * @brief A function that encapsulates the chain of operations.
     *
     * @details
     * This member stores the logic of how operations are chained together.
     * It is a std::function that takes an input Halide::Func and the coordinate Vars (x, y, c)
     * and returns the final Halide::Func representing the fused pipeline.
     * It is computed once in the constructor and reused for every `executeOnHalideBuffer`.
     */
    mutable std::function<Halide::Func(Halide::Func, Halide::Var, Halide::Var, Halide::Var)> m_operation_chain;

    /**
     * @brief Variables for the pipeline.
     */
    mutable Halide::Var m_x, m_y, m_c;

    /**
     * @brief Caches the selected backend type (CPU/GPU) to avoid runtime queries.
     */
    Common::MemoryType m_backend{Common::MemoryType::CPU_RAM};

    /**
     * @brief Flag indicating whether the operation chain has been successfully built.
     * This is false when no operations are present or if the build failed.
     */
    bool m_chain_built{false};
    // ============================================================
    // Internal Logic
    // ============================================================

    /**
     * @brief Builds the operation chain logic.
     *
     * @details
     * This method chains the operations into a single function graph.
     * It stores the resulting logic in m_operation_chain.
     */
    void buildOperationChain();

    /**
     * @brief Applies scheduling based on the cached backend type.
     *
     * @param[in,out] pipeline The Halide Func to schedule.
     * @param x The Halide Var for the x dimension.
     * @param y The Halide Var for the y dimension.
     * @param c The Halide Var for the channel dimension.
     */
    void applyScheduling(Halide::Func& pipeline, Halide::Var& x, Halide::Var& y, Halide::Var& c) const;

    // ============================================================
    // Template Implementation
    // ============================================================

    /**
     * @brief Executes pipeline on a concrete WorkingImageHalide implementation.
     *
     * @details
     * This template method expects an object derived from both WorkingImageHalide
     * and IWorkingImageHardware (e.g., WorkingImageCPU_Halide, WorkingImageGPU_Halide).
     * It accesses metadata via the IWorkingImageHardware interface and the Halide buffer
     * via the WorkingImageHalide interface.
     *
     * @tparam ConcreteImage A type that inherits from both WorkingImageHalide and IWorkingImageHardware.
     * @param[in] concrete_image The concrete working image object.
     * @return true on success, false on failure.
     */
    template<typename ConcreteImage>
    [[nodiscard]] bool executeWithConcreteHalide(
        ConcreteImage& concrete_image
        ) const;
};

} // namespace Pipeline

} // namespace CaptureMoment::Core
