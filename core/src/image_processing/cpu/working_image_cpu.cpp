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

WorkingImageCPU::WorkingImageCPU(std::unique_ptr<Common::ImageRegion> source_image)
{
    if (source_image && source_image->isValid())
    {
        auto result { initializeData(std::move(*source_image)) };

        if (!result) {
            spdlog::error("[WorkingImageCPU]: Constructor failed to initialize. Reason: {}", ErrorHandling::to_string(result.error()));
        } else {
            spdlog::debug("[WorkingImageCPU]: Constructed and initialized");
        }
    } else {
        throw std::runtime_error("Failed to initialize working image data");
    }
}

void WorkingImageCPU::resetToOriginal()
{
    if (!m_valid) {
        spdlog::warn("[WorkingImageCPU::resetToOriginal]: Cannot reset, image is invalid.");
        return;
    }

    WorkingImageData::restoreOriginalData();
    spdlog::debug("[WorkingImageCPU::resetToOriginal]: Reset working image to original data.");
}

std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError>
WorkingImageCPU::downsample(Common::ImageDim target_width, Common::ImageDim target_height)
{
    // ============================================================
    // Validate state
    if (!m_valid || m_data.empty()) {
        spdlog::warn("[WorkingImageCPU::downsample]: Image is invalid");
        return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
    }

    // Validate target dimensions
    if (target_width == 0 || target_height == 0) {
        spdlog::error("[WorkingImageCPU::downsample]: Invalid target dimensions: {}x{}",
                      target_width, target_height);
        return std::unexpected(ErrorHandling::CoreError::InvalidImageRegion);
    }

    try {

        // ============================================================
        if (m_width == target_width && m_height == target_height)
        {
            spdlog::debug("[WorkingImageCPU::downsample]: No downsample required (dimensions match). Skipping OIIO.");
            std::vector<float> data = m_data;

            auto region { std::make_unique<Common::ImageRegion>(
                std::move(data),
                static_cast<int>(m_width),
                static_cast<int>(m_height),
                static_cast<int>(m_channels)
            )};

        spdlog::debug("[WorkingImageCPU::downsample]: {}x{}x{} → {}x{}x{} (no resample)",
            m_width, m_height, m_channels,
            target_width, target_height, m_channels);
            return region;
        }

        // ============================================================
        // Step 1: Create source buffer (Zero-Copy view)
        // ============================================================
        OIIO::ImageSpec src_spec(
            static_cast<int>(m_width),
            static_cast<int>(m_height),
            static_cast<int>(m_channels),
            OIIO::TypeDesc::FLOAT
            );
        OIIO::ImageBuf src_buf(src_spec, getDataSpan().data());

        // ============================================================
        // Step 2: Create destination buffer on pre-allocated memory (Zero-Copy)
        // ============================================================

        std::vector<float> result_data(target_width * target_height * m_channels);

        OIIO::ImageSpec dst_spec(
            static_cast<int>(target_width),
            static_cast<int>(target_height),
            static_cast<int>(m_channels),
            OIIO::TypeDesc::FLOAT
            );
        OIIO::ImageBuf dst_buf(dst_spec, result_data.data());

        // ============================================================
        // Step 3: Fast resample (writes directly to result_data)
        // ============================================================
        bool success { OIIO::ImageBufAlgo::resample(dst_buf, src_buf, true) };

        if (!success || !dst_buf.initialized()) {
            spdlog::error("[WorkingImageCPU::downsample]: OIIO resample failed: {}",
                          OIIO::geterror());
            return std::unexpected(ErrorHandling::CoreError::AllocationFailed);
        }

        // ============================================================
        // Step 4: Create ImageRegion (no copy, move semantics)
        // ============================================================

        auto region { std::make_unique<Common::ImageRegion>(
            std::move(result_data),
            static_cast<int>(target_width),
            static_cast<int>(target_height),
            static_cast<int>(m_channels)
            )};

        spdlog::debug("[WorkingImageCPU::downsample]: {}x{}x{} → {}x{}x{}",
                      m_width, m_height, m_channels,
                      target_width, target_height, m_channels);

        return region;
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

} // namespace CaptureMoment::Core::ImageProcessing
