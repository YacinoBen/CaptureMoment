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
 * - **Caching**: The compiled Halide pipeline (`m_saved_pipeline`) is constructed once
 *   and reused for all subsequent executions unless the operation list changes.
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
#include "image_processing/halide/working_image_halide.h"
#include "operations/operation_descriptor.h"
#include "operations/operation_factory.h"
#include "common/types/memory_type.h"

#include <vector>
#include <memory>

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
     * The constructor performs the expensive "Just-In-Time" (JIT) compilation
     * of the pipeline. It iterates through the operations, retrieves their
     * `IOperationFusionLogic` via the factory, and constructs a fused
     * `Halide::Pipeline` object.
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
     * @brief The compiled Halide pipeline.
     *
     * @details
     * This member stores the result of the "Heavy Lifting" phase.
     * It is a compiled Halide function `fused(x,y,c)`.
     * It is computed once in the constructor and reused for every `execute`.
     */
    std::unique_ptr<Halide::Pipeline> m_saved_pipeline;

    /**
     * @brief Caches the selected backend type (CPU/GPU) to avoid runtime queries.
     */
    Common::MemoryType m_backend{Common::MemoryType::CPU_RAM};

    // ============================================================
    // Internal Logic
    // ============================================================

    /**
     * @brief Performs the "Heavy Lifting": Building the fused Halide graph.
     *
     * @details
     * This method chains the operations into a single function graph.
     * It uses the `IOperationFusionLogic` interface of each operation to append its
     * contribution to the pipeline.
     */
    void buildPipeline();

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
        ) const {
        // Validate State (using IWorkingImageHardware interface)
        if (!concrete_image.isValid()) {
            spdlog::error("[OperationPipelineExecutor] Concrete Halide Image (via HW interface) is invalid. Cannot execute.");
            return false;
        }

        // Fetch Metadata (using IWorkingImageHardware interface)
        const auto [width, height] = concrete_image.getSize();
        size_t channels = concrete_image.getChannels();

        if (width <= 0 || height <= 0 || channels == 0) {
            spdlog::error("[OperationPipelineExecutor] Invalid dimensions ({}x{}, {} ch).", width, height, channels);
            return false;
        }

        spdlog::debug("[OperationPipelineExecutor] Executing fused pipeline on {}x{}x{} image...", width, height, channels);

        // Get direct Halide buffer (Zero-Copy view of m_data) - using WorkingImageHalide interface
        // static_cast to access the WorkingImageHalide part of the ConcreteImage
        auto& halide_part = static_cast<const ImageProcessing::WorkingImageHalide&>(concrete_image);
        Halide::Buffer<float> working_buffer = halide_part.getHalideBuffer();

        if (!working_buffer.defined()) {
            spdlog::error("[OperationPipelineExecutor] Halide buffer is invalid.");
            return false;
        }

        // Execute directly on buffer using cached pipeline
        if (!m_saved_pipeline) {
            spdlog::error("[OperationPipelineExecutor] executeWithConcreteHalide: Pipeline not built. Cannot execute.");
            return false;
        }

        return executeOnHalideBuffer(working_buffer);
    }
};

} // namespace Pipeline

} // namespace CaptureMoment::Core

