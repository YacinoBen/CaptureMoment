/**
 * @file working_image_halide.cpp
 * @brief Implementation of base class for Halide-based working image implementations.
 * @author CaptureMoment Team
 * @date 2026
 */

#include "image_processing/halide/working_image_halide.h"
#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::ImageProcessing {

// ============================================================
// Helper Implementation
// ============================================================

void WorkingImageHalide::initializeHalide(const Common::ImageRegion& working_image)
{
    // Ensure data vector is sized correctly
    if (m_data.size() != working_image.m_data.size()) {
        spdlog::warn("[WorkingImageHalide] Data vector size mismatch during initialization. Resizing.");
        m_data = working_image.m_data;
    }

    // Create a Halide buffer view that references the m_data vector
    // The buffer dimensions are set, but it points directly to m_data memory (Zero-Copy view).
    m_halide_buffer = Halide::Buffer<float>(
        static_cast<int>(working_image.m_width),
        static_cast<int>(working_image.m_height),
        static_cast<int>(working_image.m_channels),
        m_data.data()
    );

    if (!m_halide_buffer.defined()) {
        spdlog::error("[WorkingImageHalide] Failed to initialize Halide::Buffer.");
    }
}

} // namespace CaptureMoment::Core::ImageProcessing
