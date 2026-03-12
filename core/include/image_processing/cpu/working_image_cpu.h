/**
 * @file working_image_cpu.h
 * @brief Image working buffers stored on the CPU.
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
#include "image_processing/common/working_image_data.h"

namespace CaptureMoment::Core {

namespace ImageProcessing {

/**
 * @class WorkingImageCPU
 * @brief Class extending IWorkingImageHardware for CPU-specific implementations.
 * This class defines the base contract for all working image implementations
 * that store their data on the CPU. Specific CPU backend implementations
 * (e.g., Halide, SIMD, OpenMP) should inherit from this interface.
 * It ensures that all CPU-based implementations provide the core functionality
 * @details
 * This class ensures that all CPU-based implementations provide the core functionality
 * defined by IWorkingImageHardware.
 */
class WorkingImageCPU : public IWorkingImageHardware, public WorkingImageData {
public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~WorkingImageCPU() = default;

    /**
     * @brief Exports a downscaled version of the image directly from GPU.
     *
     * @details
     * For CPU: Performs downsample on CPU.
     *
     * This is the preferred method for display purposes.
     */
    [[nodiscard]] virtual std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError>
    downsample(Common::ImageDim target_width, Common::ImageDim target_height) override;

protected:
    /**
     * @brief Protected constructor to enforce abstract nature.
     */
    WorkingImageCPU() = default;
};

} // namespace ImageProcessing

} // namespace CaptureMoment::Core
