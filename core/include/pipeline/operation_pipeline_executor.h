/**
 * @file operation_pipeline_executor.h
 * @brief Declaration of OperationPipelineExecutor (Fused Adjustment Pipeline).
 *
 * @details
 * This class is a concrete implementation of `IPipelineExecutor` and `IHalidePipelineExecutor`.
 * It implements the "Fused Pipeline" strategy for image adjustments using Halide.
 *
 * **Architecture Strategy:**
 * - **Inheritance**: Inherits `m_input` (4-channel Float32) from `IHalidePipelineExecutor`.
 * - **Single Compilation**: Uses `Halide::Pipeline` object to store the compiled JIT code.
 *   The compilation happens once in `init()` when operations change, not during execution.
 * - **Zero-Copy**: Execution binds the user's buffer to the inherited `m_input` and runs the pipeline.
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

#include <vector>
#include <memory>

namespace CaptureMoment::Core {

namespace Pipeline {

/**
 * @class OperationPipelineExecutor
 * @brief Concrete executor for fused Halide adjustment pipelines.
 *
 * @details
 * This executor chains multiple image adjustment operations (Brightness, Contrast, etc.)
 * into a single Halide graph. It relies on the `IHalidePipelineExecutor` base class
 * to provide the standardized 4-channel input parameter (`m_input`).
 */
class OperationPipelineExecutor final : public IPipelineExecutor, public IHalidePipelineExecutor {
public:
    /**
     * @brief Default Constructor.
     *
     * @details
     * Initializes the backend configuration (CPU/GPU) from AppConfig.
     * The `m_input` member is initialized by the base class.
     */
    OperationPipelineExecutor();

    /**
     * @brief Destructor.
     */
    virtual ~OperationPipelineExecutor() = default;

    /**
     * @brief Executes the fused pipeline on a generic working image.
     *
     * @details
     * This is the entry point for the generic `IPipelineExecutor` interface.
     * It performs a dynamic cast to determine if the image is CPU or GPU Halide-compatible
     * and dispatches to the template method `executeWithConcreteHalide`.
     *
     * @param[in,out] working_image The hardware-agnostic image to process.
     * @return true if execution succeeded, false otherwise.
     */
    [[nodiscard]] bool execute(ImageProcessing::IWorkingImageHardware& working_image) override;

    /**
     * @brief Executes the compiled pipeline directly on a raw Halide buffer (Fast Path).
     *
     * @details
     * Implements the `IHalidePipelineExecutor` interface.
     * Binds the provided buffer to the inherited `m_input` and executes the cached pipeline.
     *
     * @param[in,out] buffer The `Halide::Buffer<float>` pointing to image data (Must be 4-channel).
     * @return true if pipeline executed successfully.
     */
    [[nodiscard]] virtual bool executeOnHalideBuffer(Halide::Buffer<float>& buffer) override;

    /**
     * @brief Updates the list of operations and rebuilds the graph.
     * @details
     * Legacy method wrapper around `init`.
     */
    void updatePipeline(std::vector<Operations::OperationDescriptor> operations);

    /**
     * @brief Initializes the executor with a new list of operations.
     *
     * @details
     * This method triggers the "Heavy Lifting": building the operation graph and
     * JIT compiling the Halide pipeline. The result is stored in `m_pipeline`.
     * This should be called when the user changes parameters in the UI.
     *
     * @param operations The list of operation descriptors.
     * @param factory The operation factory reference.
     */
    void init(
        const std::vector<Operations::OperationDescriptor>& operations,
        const Operations::OperationFactory& factory
    );

    /**
     * @brief Initializes the executor with a moved list of operations (Optimized).
     */
    void init(
        std::vector<Operations::OperationDescriptor>&& operations,
        const Operations::OperationFactory& factory
    );

private:
    /**
     * @brief Stores the list of operations to be fused.
     */
    std::vector<Operations::OperationDescriptor> m_operations;

    /**
     * @brief Pointer to the operation factory.
     * @details
     * Non-owning pointer. Stored to allow dynamic creation of operation objects
     * during the graph building phase.
     */
    const Operations::OperationFactory* m_factory;

    /**
     * @brief The final output function of the fused graph.
     * @details
     * This Func depends on `m_input` (inherited).
     * It is scheduled and compiled into `m_pipeline`.
     */
    Halide::Func m_output_func;

    /**
     * @brief The compiled Halide pipeline object.
     * @details
     * Storing this allows us to execute the pipeline repeatedly without recompiling.
     */
    Halide::Pipeline m_pipeline;

    /**
     * @brief Halide variables for the pipeline dimensions.
     */
    mutable Halide::Var m_x, m_y, m_c;

    /**
     * @brief Cached backend type (CPU/GPU) from AppConfig.
     */
    Common::MemoryType m_backend{Common::MemoryType::CPU_RAM};

    /**
     * @brief Flag indicating if the pipeline has been successfully built and compiled.
     */
    bool m_chain_built{false};

    /**
     * @brief Builds the Halide function graph based on `m_operations`.
     * @details
     * Iterates through operations, creates concrete instances, and chains them
     * using `m_input` as the source.
     */
    void buildOperationChain();

    /**
     * @brief Applies scheduling directives (Vectorization/Parallelism/GPU tiling).
     * @details
     * Called during the build phase to optimize `m_output_func`.
     */
    void applyScheduling(Halide::Func& pipeline, Halide::Var& x, Halide::Var& y, Halide::Var& c) const;

    /**
     * @brief Helper template to execute on specific image types.
     *
     * @tparam ConcreteImage Type (e.g., WorkingImageCPU_Halide).
     */
    template<typename ConcreteImage>
    [[nodiscard]] bool executeWithConcreteHalide(ConcreteImage& concrete_image);
};

} // namespace Pipeline

} // namespace CaptureMoment::Core
