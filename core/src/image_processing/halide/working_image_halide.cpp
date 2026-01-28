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
    // We assume m_data was already populated via updateFromCPU (std::move).
    // We just verify that the dimensions match the metadata provided.

    const std::size_t expected_size = static_cast<std::size_t>(working_image.m_width) *
                                      working_image.m_height *
                                      working_image.m_channels;

    if (m_data.empty()) {
        spdlog::error("[WorkingImageHalide] Cannot initialize Halide buffer: Internal m_data is empty. "
                      "Did you call updateFromCPU before initializeHalide?");
        return; // Early exit, cannot create buffer
    }

    if (m_data.size() != expected_size) {
        spdlog::error("[WorkingImageHalide] Internal data size mismatch! "
                      "Expected: {}, Got: {}",
                      expected_size, m_data.size());
        // NOTE: We do NOT copy working_image.m_data here to avoid performance hits.
        // If sizes mismatch, it's a logic error in the calling code.
        return;
    }

    // --- Step 2: Create Halide Buffer View (Zero-Copy) ---
    // We link the Halide buffer directly to the existing m_data memory.
    // No allocation happens here. Halide just wraps the pointer.
    m_halide_buffer = Halide::Buffer<float>(
        m_data.data(),
        static_cast<int>(working_image.m_width),
        static_cast<int>(working_image.m_height),
        static_cast<int>(working_image.m_channels)
        );

    if (!m_halide_buffer.defined()) {
        spdlog::error("[WorkingImageHalide] Failed to define Halide::Buffer from internal data.");
    } else {
        spdlog::debug("[WorkingImageHalide] Halide buffer initialized successfully (Zero-Copy view).");
    }
}

} // namespace CaptureMoment::Core::ImageProcessing
