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
class WorkingImageCPU : public IWorkingImageHardware {
public:

    /** @brief Constructor */
    WorkingImageCPU() = default;

    /**
     * @brief Virtual destructor.
     */
    virtual ~WorkingImageCPU() = default;

    [[nodiscard]] bool isValid() const override;
    [[nodiscard]] Common::MemoryType getMemoryType() const override { return Common::MemoryType::CPU_RAM; };

    [[nodiscard]] std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError>
    downsample(Common::ImageDim target_width, Common::ImageDim target_height) override;

    /**
     * @brief Exports current internal image data to a new CPU-based ImageRegion..
     *
     * @return std::expected<std::unique_ptr<Common::ImageRegion>,  ErrorHandling::CoreError>
     */
    [[nodiscard]] std::expected<std::unique_ptr<Common::ImageRegion>,  ErrorHandling::CoreError>
    getFullResImage() override;
};

} // namespace ImageProcessing

} // namespace CaptureMoment::Core
