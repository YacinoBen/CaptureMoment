/**
 * @file i_pipeline_executor.h
 * @brief Declaration of the IPipelineExecutor interface.
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "image_processing/interfaces/i_working_image_hardware.h"

namespace CaptureMoment::Core {

namespace Pipeline {

/**
 * @interface IPipelineExecutor
 * @brief Abstract interface for executing a pre-built pipeline on an image.
 *
 * IPipelineExecutor defines the contract for any class that can execute
 * a compiled pipeline (e.g., a fused Halide pipeline) on an IWorkingImageHardware.
 * This allows for different types of pipeline executors (e.g., for adjustments,
 * erasures, filters) to be used polymorphically.
 */
class IPipelineExecutor {
public:
    /**
     * @brief Virtual destructor for safe inheritance.
     */
    virtual ~IPipelineExecutor() = default;

    /**
     * @brief Executes the pre-built pipeline on the given image.
     *
     * This pure virtual method must be implemented by derived classes to
     * execute the specific pipeline logic encapsulated by the executor.
     *
     * @param[in,out] working_image The hardware-agnostic image buffer to process.
     *                              The image is modified in-place by the pipeline.
     * @return true if the execution was successful, false otherwise
     *         (e.g., runtime error during execution).
     */
    [[nodiscard]] virtual bool execute(ImageProcessing::IWorkingImageHardware& working_image) const = 0;
};

} // namespace Pipeline

} // namespace CaptureMoment::Core
