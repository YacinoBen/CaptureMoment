/**
 * @file image_conversion.cpp
 * @brief Implementation of image conversion utilities
 * @author CaptureMoment Team
 * @date 2026
 */

#include "utils/image_conversion.h"
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::Utils {

std::unique_ptr<Common::ImageRegion> convert_RGBA_F32_to_RGBA_U8(const Common::ImageRegion& input)
{
    if (!input.isValid() || input.m_channels != 4 || input.m_format != Common::PixelFormat::RGBA_F32) {
        spdlog::error("convert_RGBA_F32_to_RGBA_U8: Input is invalid or not RGBA_F32");
        return nullptr;
    }

    // 1. Setup OIIO Source Buffer (Zero-Copy)
    OIIO::ImageSpec src_spec(input.m_width, input.m_height, input.m_channels, OIIO::TypeDesc::FLOAT);
    OIIO::ImageBuf src_buf(src_spec, const_cast<float*>(input.m_data.data()));

    // 2. Setup OIIO Destination Spec
    OIIO::ImageSpec dst_spec(input.m_width, input.m_height, input.m_channels, OIIO::TypeDesc::UINT8);
    OIIO::ImageBuf dst_buf(dst_spec);

    // 3. Perform Conversion (F32 -> U8) using OIIO
    if (!OIIO::ImageBufAlgo::copy(dst_buf, src_buf)) {
        spdlog::error("convert_RGBA_F32_to_RGBA_U8: OIIO conversion failed");
        return nullptr;
    }

    // 4. Extract U8 data to ImageRegion
    auto result = std::make_unique<Common::ImageRegion>();
    result->m_x = input.m_x;
    result->m_y = input.m_y;
    result->m_width = input.m_width;
    result->m_height = input.m_height;
    result->m_channels = input.m_channels;
    result->m_format = Common::PixelFormat::RGBA_U8;

    const size_t total_elements = static_cast<size_t>(input.m_width) * input.m_height * input.m_channels;
    result->m_data.resize(total_elements);

    if (!dst_buf.get_pixels(OIIO::ROI::All(), OIIO::TypeDesc::UINT8, result->m_data.data())) {
        spdlog::error("convert_RGBA_F32_to_RGBA_U8: Failed to extract U8 pixels from OIIO buffer");
        return nullptr;
    }

    return result;
}

} // namespace CaptureMoment::Core::Utils
