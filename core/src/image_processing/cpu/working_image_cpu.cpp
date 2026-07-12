/**
 * @file working_image_cpu.cpp
 * @brief Implementation of WorkingImageCPU.
 * @author CaptureMoment Team
 * @date 2026
 */

#include "image_processing/cpu/working_image_cpu.h"
#include "common/error_handling/core_error.h"

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::ImageProcessing {

bool WorkingImageCPU::isValid() const
{
    return !m_view_data_image.working_data.empty() && m_view_data_image.width > 0;
}

std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError>
WorkingImageCPU::downsample(Common::ImageDim target_width, Common::ImageDim target_height)
{
    if (!isValid() || m_view_data_image.working_data.empty()) {
        spdlog::warn("[WorkingImageCPU::downsample]: Image view is invalid");
        return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
    }

    if (target_width == 0 || target_height == 0) {
        spdlog::error("[WorkingImageCPU::downsample]: Target dimensions are invalid ({}x{})", target_width, target_height);
        return std::unexpected(ErrorHandling::CoreError::InvalidImageRegion);
    }

    try {

        spdlog::debug("[WorkingImageCPU::downsample]: Start downsample.");

        // ============================================================
        if (m_view_data_image.width == target_width && m_view_data_image.height == target_height)
        {
            spdlog::debug("[WorkingImageCPU::downsample]: No downsample required.");

            return std::make_unique<Common::ImageRegion>(
                m_view_data_image.working_data,
                static_cast<int>(m_view_data_image.width),
                static_cast<int>(m_view_data_image.height),
                static_cast<int>(m_view_data_image.channels)
            );
        }

        // ============================================================
        // Step 1: Create source buffer (Zero-Copy view via m_view_data_image)
        // ============================================================
        OIIO::ImageSpec src_spec(
            static_cast<int>(m_view_data_image.width),
            static_cast<int>(m_view_data_image.height),
            static_cast<int>(m_view_data_image.channels),
            OIIO::TypeDesc::FLOAT
            );

        OIIO::ImageBuf src_buf(src_spec, m_view_data_image.working_data.data());

        // ============================================================
        // Step 2 & 3: Allocate dest and Resample
        // ============================================================
        std::vector<float> result_data(target_width * target_height * m_view_data_image.channels);

        OIIO::ImageSpec dst_spec(
            static_cast<int>(target_width),
            static_cast<int>(target_height),
            static_cast<int>(m_view_data_image.channels),
            OIIO::TypeDesc::FLOAT
            );
        OIIO::ImageBuf dst_buf(dst_spec, result_data.data());

        if (!OIIO::ImageBufAlgo::resample(dst_buf, src_buf, false)) {
            spdlog::error("[WorkingImageCPU::downsample]: OIIO resample failed: {}", OIIO::geterror());
            return std::unexpected(ErrorHandling::CoreError::AllocationFailed);
        }

        // ============================================================
        // Step 4: Create ImageRegion (move semantics)
        // ============================================================

        spdlog::debug("WorkingImageCPU::downsample]: Downsample successful, creating ImageRegion from {}x{} to {}x{}.",
                      m_view_data_image.width, m_view_data_image.height, target_width, target_height);
        return std::make_unique<Common::ImageRegion>(
            std::move(result_data),
            static_cast<int>(target_width),
            static_cast<int>(target_height),
            static_cast<int>(m_view_data_image.channels)
        );
    }
    catch (const std::bad_alloc& e) {
        spdlog::critical("[WorkingImageCPU::downsample]: Allocation failed: {}", e.what());
        return std::unexpected(ErrorHandling::CoreError::AllocationFailed);
    }
    catch (const std::exception& e) {
        spdlog::critical("[WorkingImageCPU::downsample]: Exception: {}", e.what());
        return std::unexpected(ErrorHandling::CoreError::Unexpected);
    }
}

std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError>
WorkingImageCPU::getFullResImage()
{
   if (!isValid()) {
        spdlog::warn("[WorkingImageCPU::getFullResImage]: Current image is invalid, cannot export");
        return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
    }

    try {
        auto cpu_image_copy { std::make_unique<Common::ImageRegion>() };


        cpu_image_copy->m_width = static_cast<int>(m_view_data_image.width);
        cpu_image_copy->m_height = static_cast<int>(m_view_data_image.height);
        cpu_image_copy->m_channels = static_cast<int>(m_view_data_image.channels);
        cpu_image_copy->m_format = Common::PixelFormat::RGBA_F32;

        cpu_image_copy->m_data.assign(m_view_data_image.working_data.begin(), m_view_data_image.working_data.end());

        if (!cpu_image_copy->isValid()) {
            return std::unexpected(ErrorHandling::CoreError::InvalidImageRegion);
        }

        spdlog::debug("[WorkingImageCPU::getFullResImage]: Exported ImageRegion ({}x{}, {} ch)",
                      cpu_image_copy->m_width, cpu_image_copy->m_height, cpu_image_copy->m_channels);

        return cpu_image_copy;

    } catch (const std::bad_alloc& e) {
        spdlog::critical("[WorkingImageCPU::getFullResImage]: Allocation failed: {}", e.what());
        return std::unexpected(ErrorHandling::CoreError::AllocationFailed);
    } catch (const std::exception& e) {
        spdlog::critical("[WorkingImageCPU::getFullResImage]: Exception: {}", e.what());
        return std::unexpected(ErrorHandling::CoreError::Unexpected);
    }
}


} // namespace CaptureMoment::Core::ImageProcessing
