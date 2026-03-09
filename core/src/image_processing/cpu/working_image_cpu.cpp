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

std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError>
WorkingImageCPU::downsample(size_t target_width, size_t target_height)
{
    // Validate state
    if (!m_valid || !m_data || m_data_size == 0) {
        spdlog::warn("[WorkingImageCPU::downsample] Image is invalid");
        return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
    }

    // Validate target dimensions
    if (target_width == 0 || target_height == 0) {
        spdlog::error("[WorkingImageCPU::downsample] Invalid target dimensions: {}x{}",
                      target_width, target_height);
        return std::unexpected(ErrorHandling::CoreError::InvalidImageRegion);
    }

    try {
        // ============================================================
        // Step 1: Create OIIO buffer wrapping existing data (Zero-Copy view)
        // ============================================================
        OIIO::ImageSpec src_spec(
            static_cast<int>(m_width),
            static_cast<int>(m_height),
            static_cast<int>(m_channels),
            OIIO::TypeDesc::FLOAT
            );
        // OIIO ImageBuf takes a pointer to external memory - NO COPY of source
        OIIO::ImageBuf src_buf(src_spec, getDataSpan().data());

        // ============================================================
        // Step 2: Define target region of interest
        // ============================================================
        OIIO::ROI target_roi(
            0, static_cast<int>(target_width),
            0, static_cast<int>(target_height),
            0, 1,
            0, static_cast<int>(m_channels)
            );

        // ============================================================
        // Step 3: Perform resample (bilinear interpolation)
        // OIIO reads from our buffer directly - only writes to new result
        // ============================================================
        OIIO::ImageBuf result_buf = OIIO::ImageBufAlgo::resample(src_buf, true, target_roi);

        if (!result_buf.initialized()) {
            spdlog::error("[WorkingImageCPU::downsample]: OIIO resample failed");
            return std::unexpected(ErrorHandling::CoreError::AllocationFailed);
        }

        // ============================================================
        // Step 4: Allocate result vector and extract pixels
        // ============================================================
        const size_t result_size = target_width * target_height * m_channels;
        std::vector<float> result_data(result_size);

        if (!result_buf.get_pixels(OIIO::ROI::All(), OIIO::TypeDesc::FLOAT, result_data.data())) {
            spdlog::error("[WorkingImageCPU::downsample]: Failed to extract pixels from OIIO buffer");
            return std::unexpected(ErrorHandling::CoreError::AllocationFailed);
        }

        // ============================================================
        // Step 5: Create ImageRegion with result
        // NOTE: COPY semantics - m_data remains VALID
        // WorkingImage can be downsampled multiple times
        // ============================================================
        auto region = std::make_unique<Common::ImageRegion>(
            std::move(result_data),
            static_cast<int>(target_width),
            static_cast<int>(target_height),
            static_cast<int>(m_channels)
            );

        spdlog::debug("[WorkingImageCPU::downsample]: {}x{} → {}x{} (COPY, image remains valid)",
                      m_width, m_height, target_width, target_height);

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
