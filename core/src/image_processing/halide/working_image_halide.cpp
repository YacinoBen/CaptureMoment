/**
 * @file working_image_halide.cpp
 * @brief Implementation of base class for Halide-based working image implementations.
 * @author CaptureMoment Team
 * @date 2026
 */

#include "image_processing/halide/working_image_halide.h"
#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::ImageProcessing {

void WorkingImageHalide::initializeHalide(const Common::ImageRegion& working_image)
{
      // --- Step 1: Validation of existing data ---
    const std::size_t expected_size = static_cast<std::size_t>(working_image.m_width) *
                                      working_image.m_height *
                                      working_image.m_channels;

    if (!m_data) {
        spdlog::error("[WorkingImageHalide] Cannot initialize Halide buffer: Internal m_data is null.");
        return;
    }

    if (m_data_size != expected_size) {
        spdlog::error("[WorkingImageHalide] Internal data size mismatch! "
                      "Expected: {}, Got: {}. Did you call updateFromCPU before initializeHalide?",
                      expected_size, m_data_size);
        return;
    }

    // --- Step 2: Create Halide Buffer View (Zero-Copy) ---
    // Link the Halide buffer directly to the existing raw pointer in m_data.
    // No allocation happens here.
    m_halide_buffer = Halide::Buffer<float>(
        m_data.get(), // Use raw pointer from unique_ptr
        static_cast<int>(working_image.m_width),
        static_cast<int>(working_image.m_height),
        static_cast<int>(working_image.m_channels)
    );

    if (!m_halide_buffer.defined()) {
        spdlog::error("[WorkingImageHalide] Failed to define Halide::Buffer from internal data.");
    } else {
        spdlog::debug("[WorkingImageHalide] Halide buffer initialized successfully (Zero-Copy view on {} elements).", m_data_size);
    }
}

} // namespace CaptureMoment::Core::ImageProcessing
