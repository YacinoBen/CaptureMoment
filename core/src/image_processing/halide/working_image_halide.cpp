/**
 * @file working_image_halide.cpp
 * @brief Implementation of base class for Halide-based working image implementations.
 * @author CaptureMoment Team
 * @date 2026
 */

#include "image_processing/halide/working_image_halide.h"
#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::ImageProcessing {

void WorkingImageHalide::initializeHalide(std::span<float> data, std::int32_t width, std::int32_t height, std::int32_t channels)
{
    if (data.empty()) {
        spdlog::error("[WorkingImageHalide::initializeHalide]: Cannot initialize Halide buffer: data span is empty.");
        return;
    }

    // Create Halide Buffer View (Zero-Copy)
    // std::span::data() returns the underlying pointer safely.
    m_halide_buffer = Halide::Buffer<float>(data.data(), width, height, channels);

    if (!m_halide_buffer.defined()) {
        spdlog::error("[WorkingImageHalide::initializeHalide]: Failed to define Halide::Buffer.");
    } else {
        spdlog::debug("[WorkingImageHalide::initializeHalide]: Halide buffer initialized ({}x{}, {} ch, zero-copy).",
                      width, height, channels);
    }
}

std::pair<Common::ImageDim, Common::ImageDim> WorkingImageHalide::getSizeByHalide() const noexcept
{
    if (!m_halide_buffer.defined()) {
            return {0, 0};
        }
    return {static_cast<Common::ImageDim>(m_halide_buffer.width()),
                static_cast<Common::ImageDim>(m_halide_buffer.height())};
}

Common::ImageChan WorkingImageHalide::getChannelsByHalide() const noexcept
{
    if (!m_halide_buffer.defined()) {
        return 0;
    }
    return static_cast<Common::ImageChan>(m_halide_buffer.channels());
}

Common::ImageSize WorkingImageHalide::getPixelCountByHalide() const noexcept
{
    if (!m_halide_buffer.defined()) {
        return 0;
    }
    return static_cast<Common::ImageSize>(m_halide_buffer.width()) * static_cast<Common::ImageSize>(m_halide_buffer.height());
}

Common::ImageSize WorkingImageHalide::getDataSizeByHalide() const noexcept
{
    if (!m_halide_buffer.defined()) {
        return 0;
    }
    return static_cast<Common::ImageSize>(m_halide_buffer.size_in_bytes() / sizeof(float));
}

} // namespace CaptureMoment::Core::ImageProcessing
