/**
 * @file i_working_image_cpu.h
 * @brief Abstract interface for image working buffers stored on the CPU.
 *
 * @details
 * This interface defines the base contract for all working image implementations
 * that store their data on the CPU. Specific CPU backend implementations
 * (e.g., Default Vector, Halide) should inherit from this interface.
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "image_processing/interfaces/i_working_image_hardware.h"

namespace CaptureMoment::Core {

namespace ImageProcessing {

/**
 * @interface IWorkingImageCPU
 * @brief Abstract interface extending IWorkingImageHardware for CPU-specific implementations.
 * This interface defines the base contract for all working image implementations
 * that store their data on the CPU. Specific CPU backend implementations
 * (e.g., Halide, SIMD, OpenMP) should inherit from this interface.
 * It ensures that all CPU-based implementations provide the core functionality
 * @details
 * This class ensures that all CPU-based implementations provide the core functionality
 * defined by IWorkingImageHardware.
 */
class IWorkingImageCPU : public IWorkingImageHardware {
public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~IWorkingImageCPU() = default;

    // Inherits all methods from IWorkingImageHardware.
    // No CPU-specific methods needed at this abstraction level.

protected:
    /**
     * @brief Protected constructor to enforce abstract nature.
     */
    IWorkingImageCPU() = default;
};

} // namespace ImageProcessing

} // namespace CaptureMoment::Core
