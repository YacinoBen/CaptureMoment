/**
 * @file working_image_gpu.cpp
 * @brief Implementation of WorkingImageGPU.
 * @author CaptureMoment Team
 * @date 2026
 */

#include "image_processing/gpu/working_image_gpu.h"

#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::ImageProcessing {

void WorkingImageGPU::resetToOriginal()
{
    if (!m_valid) {
        spdlog::warn("[WorkingImageGPU::resetToOriginal]: Cannot reset, image is invalid.");
        return;
    } 
    
    WorkingImageData::restoreOriginalData();
    spdlog::debug("[WorkingImageGPU::resetToOriginal]: Reset working image to original data.");
}

} // namespace CaptureMoment::Core::ImageProcessing
