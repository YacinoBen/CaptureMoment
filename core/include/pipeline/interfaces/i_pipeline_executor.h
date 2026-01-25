/**
 * @file i_pipeline_executor.h
 * @brief Abstract interface for executing a compiled image processing pipeline.
 *
 * @details
 * This interface defines the contract for running a pre-built (fused)
 * Halide pipeline on an image. It acts as the "Runner".
 *
 * **Usage:**
 * @code
 * auto executor = OperationPipelineBuilder::build(ops, factory);
 * executor->execute(image_buffer); // Generic
 * @endcode
 *
 * **Extensibility:**
 * Concrete implementations can implement hardware-specific paths
 * (e.g., `IHalidePipelineExecutor` for direct memory access).
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

namespace CaptureMoment::Core {

namespace Pipeline {

/**
 * @interface IPipelineExecutor
 * @brief Abstract interface for executing a pre-compiled Halide pipeline.
 *
 * @details
 * This interface separates the *construction* and *caching* of a pipeline
 * (handled by `OperationPipelineBuilder` and the executor) from the *execution* phase.
 * It ensures that the heavy lifting (JIT compilation, scheduling) happens
 * only once (or when operations change), while the actual run
 * (Halide::Func::realize) can be called repeatedly at maximum speed.
 *
 * Implementations are expected to handle the specific scheduling
 * (CPU vectorization vs GPU tiling) required by the target hardware.
 */
class IPipelineExecutor {
public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~IPipelineExecutor() = default;

    /**
     * @brief Executes the compiled pipeline on a generic working image.
     *
     * @details
     * This is the standard entry point. Concrete implementations (like
     * `OperationPipelineExecutor`) may attempt to cast the image to a more
     * specific type (e.g., `WorkingImageCPU_Halide`) to access the raw
     * Halide buffer directly (Fast Path).
     *
     * @param[in,out] working_image The hardware-agnostic image containing the pixel data.
     * @return true if pipeline executed successfully, false otherwise.
     */
    [[nodiscard]] virtual bool execute(ImageProcessing::IWorkingImageHardware& working_image) = 0;

protected:
    /**
     * @brief Protected constructor to enforce abstract nature.
     */
    IPipelineExecutor() = default;
};
} // namespace Pipeline

} // namespace CaptureMoment::Core
