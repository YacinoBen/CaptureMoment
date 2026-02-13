/**
 * @file working_image_registration.cpp
 * @brief Implementation of backend registration.
 *
 * Concrete backend headers are included here to keep them out of public headers.
 * @author CaptureMoment Team
 * @date 2026
 */

#include "image_processing/registration/working_image_registration.h"
#include "image_processing/factories/working_image_factory.h"

// Concrete Implementations
#include "image_processing/cpu/working_image_cpu_halide.h"
#include "image_processing/gpu/working_image_gpu_halide.h"

#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::ImageProcessing {

void registerDefaultBackends()
{
    spdlog::info("[WorkingImageRegistration] Registering default Core backends...");

    // 1. Register CPU Backend (Halide)
    WorkingImageFactory::registerCreator(
        Common::MemoryType::CPU_RAM,
        [](const Common::ImageRegion& img) {
            spdlog::debug("[WorkingImageRegistration] Creating CPU Halide Backend");
            // Note: WorkingImageCPU_Halide takes unique_ptr, so we wrap the copy in one.
            return std::make_unique<WorkingImageCPU_Halide>(std::make_unique<Common::ImageRegion>(img));
        }
    );

    // 2. Register GPU Backend (Halide)
    WorkingImageFactory::registerCreator(
        Common::MemoryType::GPU_MEMORY,
        [](const Common::ImageRegion& img) {
            spdlog::debug("[WorkingImageRegistration] Creating GPU Halide Backend");
            return std::make_unique<WorkingImageGPU_Halide>(std::make_unique<Common::ImageRegion>(img));
        }
    );

    spdlog::info("[WorkingImageRegistration] Default backends registration complete.");
}

} // namespace CaptureMoment::Core::ImageProcessing
