/**
 * @file i_halide_pipeline_executor.h
 * @brief Specialized interface for Halide-optimized pipeline executors.
 *
 * @details
 * This interface extends `IPipelineExecutor` with a specific method
 * `executeOnHalideBuffer()`. This is necessary because the Halide
 * implementation requires direct access to the raw `Halide::Buffer` object to
 * execute the fused pipeline efficiently without wrapping it in a generic
 * `IWorkingImageHardware`.
 *
 * This allows executors to implement the most optimized path possible:
 * - **Direct Buffer Access**: Accessing the buffer directly avoids virtual function
 *   and potential `getHalideBuffer()` overhead.
 * - **Halide-Specific**: Can rely on specific Halide features (e.g., specific scheduling)
 *   that might not be available in generic `IWorkingImageHardware` interface.
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include <Halide.h>

namespace CaptureMoment::Core {

namespace Pipeline {

/**
 * @interface IHalidePipelineExecutor
 * @brief Specialized interface for Halide-optimized pipeline executors.
 *
 * @details
 * This interface extends `IPipelineExecutor` and provides a method
 * to execute pipelines directly on a raw `Halide::Buffer`.
 *
 * Implementations (e.g., `OperationPipelineExecutor`) should prioritize this method
 * over the generic `execute()` method when possible to maximize performance.
 */
class IHalidePipelineExecutor {
public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~IHalidePipelineExecutor() = default;

    /**
     * @brief Executes the compiled pipeline directly on a Halide buffer.
     *
     * @details
     * This method is the "Fast Path" for Halide pipelines.
     * It takes a raw `Halide::Buffer<float>` which points directly to the image
     * memory (e.g., `WorkingImageHalide::m_data`).
     *
     * Because we pass the raw buffer, we avoid:
     * 1. Virtual function call overhead.
     * 2. Generic accessors like `working_image.getHalideBuffer()` (which might
     *   implement thread-safety checks or other logic).
     *
     * @param[in,out] buffer The Halide buffer pointing to image data. The pipeline is applied here.
     * @return true if pipeline executed successfully, false if an error occurred.
     */
    [[nodiscard]] virtual bool executeOnHalideBuffer(Halide::Buffer<float>& buffer) const = 0;
};

} // namespace Pipeline

} // namespace CaptureMoment::Core::ImageProcessing
