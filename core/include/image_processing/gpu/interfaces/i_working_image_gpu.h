/**
 * @file i_working_image_gpu.h
 * @brief Abstract interface for image working buffers stored on the GPU.
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "image_processing/interfaces/i_working_image_hardware.h"

#include <memory>
#include <cstddef>

namespace CaptureMoment::Core {
    
namespace ImageProcessing {

/**
 * @brief Abstract interface extending IWorkingImageHardware for GPU-specific implementations.
 *
 * This interface defines the base contract for all working image implementations
 * that store their data on the GPU. Specific GPU backend implementations
 * (e.g., Halide GPU, CUDA, OpenCL) should inherit from this interface.
 * It ensures that all GPU-based implementations provide the core functionality
 * defined by IWorkingImageHardware.
 */
class IWorkingImageGPU : public IWorkingImageHardware {
public:
    /**
     * @brief Virtual destructor for safe inheritance and polymorphic deletion.
     */
    virtual ~IWorkingImageGPU() = default;

    // Inherit all methods from IWorkingImageHardware.
    // Potentially add GPU-specific methods here in the future if needed,
    // but for now, it serves as a marker interface for GPU implementations.

protected:
    // Protected constructor to enforce abstract nature.
    IWorkingImageGPU() = default;
};

} // namespace ImageProcessing

} // namespace CaptureMoment::Core

