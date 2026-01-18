/**
 * @file working_image_factory.cpp
 * @brief Implementation of WorkingImageFactory
 * @author CaptureMoment Team
 * @date 2026
 */

#include "image_processing/factories/working_image_factory.h"
#include "image_processing/image_processing.h" 
#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::ImageProcessing {

std::unique_ptr<IWorkingImageHardware> WorkingImageFactory::create(
    Common::MemoryType backend,
    const Common::ImageRegion& source_image
) {
    std::unique_ptr<IWorkingImageHardware> img;

    // Select the appropriate implementation based on the backend
    if (backend == Common::MemoryType::CPU_RAM) {
        img = std::make_unique<WorkingImageCPU_Halide>();
    } else {
        img = std::make_unique<WorkingImageGPU_Halide>();
    }

    // Initialize the new image with the source data
    if (!img->updateFromCPU(source_image)) {
        spdlog::error("WorkingImageFactory::create: Failed to initialize working image from source.");
        return nullptr;
    }

    return img;
}

} // namespace CaptureMoment::Core::ImageProcessing
