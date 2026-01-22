/**
 * @file working_image_halide.cpp
 * @brief Implementation of WorkingImageHalide
 * @author CaptureMoment Team
 * @date 2026
 */

#include "image_processing/halide/working_image_halide.h"

#include <spdlog/spdlog.h>
#include <memory>
#include <cstring>

namespace CaptureMoment::Core::ImageProcessing {

void WorkingImageHalide::initializeHalide(const Common::ImageRegion& working_image)
{
    if (m_data.empty()) {
        spdlog::error("WorkingImageHalide::initializeHalide: Internal data vector is empty, cannot initialize buffer");
        return;
    }

    m_halide_buffer = Halide::Buffer<float>(
        m_data.data(),
        working_image.m_width,
        working_image.m_height,
        working_image.m_channels
        );
}

} // namespace CaptureMoment::Core::ImageProcessing
