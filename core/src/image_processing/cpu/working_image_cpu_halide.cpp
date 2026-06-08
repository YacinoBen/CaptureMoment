/**
 * @file working_image_cpu_halide.cpp
 * @brief Implementation of WorkingImageCPU_Halide
 * @author CaptureMoment Team
 * @date 2025
 */

#include "image_processing/cpu/working_image_cpu_halide.h"

#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::ImageProcessing {

bool WorkingImageCPU_Halide::bindView(const ImageView& view)
{
    // 1. Bind the view to the CPU base class (stores spans and geometry)
    if (!WorkingImageCPU::bindView(view)) {
        return false;
    }

    // 2. Check the view data pointer before initializing Halide
    if (m_view_data_image.working_data.data() == nullptr) {
        spdlog::error("[WorkingImageCPU_Halide::bindView]: View data pointer is null.");
        return false;
    }

    // 3. Initialize the Halide buffer to reference the CPU data (zero-copy)
    initializeHalide(m_view_data_image.working_data, m_view_data_image.width, m_view_data_image.height, m_view_data_image.channels);

    if (!m_halide_buffer.defined()) {
        spdlog::error("[WorkingImageCPU_Halide::bindView]: Failed to initialize Halide buffer.");
        return false;
    }

    spdlog::debug("[WorkingImageCPU_Halide::bindView]: Bound and initialized Halide ({}x{}, {} ch).",
                  m_view_data_image.width, m_view_data_image.height, m_view_data_image.channels);
    return true;
}

std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError>
WorkingImageCPU_Halide::convertHalideToImageRegion()
{
 try {
        auto cpu_image_copy { std::make_unique<Common::ImageRegion>() };

        auto [w, h] = getSizeByHalide();
        cpu_image_copy->m_width = static_cast<int>(w);
        cpu_image_copy->m_height = static_cast<int>(h);
        cpu_image_copy->m_channels = static_cast<int>(getChannelsByHalide());
        cpu_image_copy->m_format = Common::PixelFormat::RGBA_F32;

        if (m_view_data_image.working_data.empty()) {
            return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
        }
        cpu_image_copy->m_data.assign(m_view_data_image.working_data.begin(), m_view_data_image.working_data.end());

        if (!cpu_image_copy->isValid()) {
            return std::unexpected(ErrorHandling::CoreError::InvalidImageRegion);
        }

        spdlog::debug("[WorkingImageCPU_Halide::convertHalideToImageRegion]: Exported ImageRegion ({}x{}, {} ch)",
                      cpu_image_copy->m_width, cpu_image_copy->m_height, cpu_image_copy->m_channels);

        return cpu_image_copy;

    } catch (const std::bad_alloc& e) {
        spdlog::critical("[WorkingImageCPU_Halide::convertHalideToImageRegion]: Allocation failed: {}", e.what());
        return std::unexpected(ErrorHandling::CoreError::AllocationFailed);
    } catch (const std::exception& e) {
        spdlog::critical("[WorkingImageCPU_Halide::convertHalideToImageRegion]: Exception: {}", e.what());
        return std::unexpected(ErrorHandling::CoreError::Unexpected);
    }
}

std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError>
WorkingImageCPU_Halide::exportToCPUCopy()
{
    if (!isValid()) {
        spdlog::warn("[WorkingImageCPU_Halide::exportToCPUCopy]: Current Halide buffer is invalid, cannot export");
        return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
    }

    return convertHalideToImageRegion();
}

} // namespace CaptureMoment::Core::ImageProcessing
