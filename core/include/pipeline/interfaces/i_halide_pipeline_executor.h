/**
 * @interface IHalidePipelineExecutor
 * @brief Specialized interface for executors capable of "Fast Path" direct execution.
 *
 * @details
 * This interface extends `IPipelineExecutor` by adding a method that accepts a
 * raw `Halide::Buffer` reference. This allows executors that manage
 * `WorkingImageHalide` (which wraps a Halide buffer) to execute the pipeline
 * without copying data out of the managed vector or validating generic wrappers.
 *
 * Implementations of this interface can perform strictly necessary checks and then
 * invoke the compiled Halide function, maximizing performance for high-frequency calls.
 */

#pragma once
#include "core/include/pipeline/interfaces/i_pipeline_executor.h"
#include <HalideBuffer.h>

class IHalidePipelineExecutor : public IPipelineExecutor {
public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~IHalidePipelineExecutor() = default;

    /**
     * @brief Executes the compiled pipeline directly on a raw Halide buffer.
     *
     * @details
     * This "Fast Path" bypasses generic `getHalideBuffer()` or `export/import` logic.
     * It assumes the passed `Halide::Buffer` points to valid memory of the correct dimensions.
     *
     * @param[in,out] buffer The Halide buffer pointing to the image data.
     * @return true if pipeline executed successfully, false otherwise.
     */
    [[nodiscard]] virtual bool executeOnHalideBuffer(Halide::Buffer<float>& buffer) const = 0;
};

} // namespace Pipeline

} // namespace CaptureMoment::Core
