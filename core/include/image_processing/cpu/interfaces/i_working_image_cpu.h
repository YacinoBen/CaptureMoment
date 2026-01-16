/**
 * @file i_working_image_cpu.h
 * @brief Abstract interface for image working buffers stored on the CPU.
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "image_processing/interfaces/i_working_image_hardware.h"
#include "common/image_region.h"

#include <memory>
#include <cstddef>

namespace CaptureMoment::Core {

namespace ImageProcessing {

/**
 * @brief Abstract interface extending IWorkingImageHardware for CPU-specific implementations.
 *
 * This interface defines the base contract for all working image implementations
 * that store their data on the CPU. Specific CPU backend implementations
 * (e.g., Halide, SIMD, OpenMP) should inherit from this interface.
 * It ensures that all CPU-based implementations provide the core functionality
 * defined by IWorkingImageHardware.
 */
class IWorkingImageCPU : public IWorkingImageHardware {
public:
    /**
     * @brief Virtual destructor for safe inheritance and polymorphic deletion.
     */
    virtual ~IWorkingImageCPU() = default;

    // Inherit all methods from IWorkingImageHardware.
    // Potentially add CPU-specific methods here in the future if needed,
    // but for now, it serves as a marker interface for CPU implementations.

protected:
    // Protected constructor to enforce abstract nature.
    IWorkingImageCPU() = default;
};

} // namespace ImageProcessing

} // namespace CaptureMoment::Core
