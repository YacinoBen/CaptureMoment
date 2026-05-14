/**
 * @file working_image_gpu.h
 * @brief Image working buffers stored on the GPU
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
 * @brief Class extending IWorkingImageHardware for GPU-specific implementations.
 *
 * @details
 * This interface defines the base contract for all working image implementations
 * that store their data on the GPU. Specific GPU backend implementations
 * (e.g., Halide GPU, CUDA, OpenCL) should inherit from this interface.
 * It ensures that all GPU-based implementations provide the core functionality
 * defined by IWorkingImageHardware.
 */
class WorkingImageGPU : public IWorkingImageHardware {
public:
    /** @brief Constructor */
    WorkingImageGPU() = default;

    /**
     * @brief Virtual destructor for safe inheritance and polymorphic deletion.
     */
    virtual ~WorkingImageGPU() = default;

    /**
     * @brief Checks if the GPU buffer is valid and allocated.
     */
    [[nodiscard]] bool isValid() const override;
    
    /**
     * @brief Gets the memory type where data resides.
     */
    [[nodiscard]] Common::MemoryType getMemoryType() const override {
        return Common::MemoryType::GPU_MEMORY;
    }
};

} // namespace ImageProcessing

} // namespace CaptureMoment::Core
