/**
 * @file working_image_factory.h
 * @brief Factory for creating IWorkingImageHardware instances from an ImageRegion.
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "image_processing/interfaces/i_working_image_hardware.h"
#include "common/image_region.h"

namespace CaptureMoment::Core {

namespace ImageProcessing {

/**
 * @brief Factory class for creating concrete IWorkingImageHardware instances.
 *
 * This factory encapsulates the logic for instantiating the correct
 * hardware-specific implementation (e.g., CPU or GPU) of a working image
 * based on a given MemoryType and source image data.
 *
 * It provides a single, centralized point for this creation logic,
 * preventing code duplication in classes like StateImageManager and PhotoTask.
 * The factory is stateless and consists only of static methods.
 */
class WorkingImageFactory {
public:
    /**
     * @brief Creates a new IWorkingImageHardware instance from a source ImageRegion.
     *
     * This method selects the appropriate concrete implementation
     * (e.g., WorkingImageCPU_Halide, WorkingImageGPU_Halide) based on the
     * provided backend type. It then initializes the new working image
     * with the data from the source_image.
     *
     * @param backend The desired hardware backend (MemoryType::CPU_RAM or MemoryType::GPU_MEMORY).
     * @param source_image The source image data to be copied into the new working image.
     * @return A unique pointer to the newly created and initialized IWorkingImageHardware.
     *         Returns nullptr if the creation or initialization fails (e.g., invalid source image,
     *         unsupported backend, or GPU memory allocation failure).
     */
    [[nodiscard]] static std::unique_ptr<ImageProcessing::IWorkingImageHardware> create(
        Common::MemoryType backend,
        const Common::ImageRegion& source_image
    );
};

} // namespace ImageProcessing

} // namespace CaptureMoment::Core
