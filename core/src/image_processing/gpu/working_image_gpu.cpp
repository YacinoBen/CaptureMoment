/**
 * @file working_image_gpu.cpp
 * @brief Implementation of WorkingImageGPU.
 * @author CaptureMoment Team
 * @date 2026
 */

#include "image_processing/gpu/working_image_gpu.h"

#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::ImageProcessing {

bool WorkingImageGPU::isValid() const
{
    return !m_view_data_image.working_data.empty() && m_view_data_image.width > 0;
}

} // namespace CaptureMoment::Core::ImageProcessing
