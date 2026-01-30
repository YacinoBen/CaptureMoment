/**
 * @file working_image_cpu_halide.cpp
 * @brief Implementation of WorkingImageCPU_Halide
 * @author CaptureMoment Team
 * @date 2025
 */

#include "image_processing/cpu/working_image_cpu_halide.h"

#include <spdlog/spdlog.h>
#include <utility>
#include <cstring>

namespace CaptureMoment::Core::ImageProcessing {

WorkingImageCPU_Halide::WorkingImageCPU_Halide(std::unique_ptr<Common::ImageRegion> initial_image)
{
    if (initial_image && initial_image->isValid()) {
        // Use move semantics to initialize efficiently
        auto result = updateFromCPU(std::move(*initial_image));

        if (!result) {
            spdlog::error("WorkingImageCPU_Halide: Constructor failed to initialize. Reason: {}", ErrorHandling::to_string(result.error()));
        } else {
            spdlog::debug("WorkingImageCPU_Halide: Constructed and initialized with valid initial image ({}x{}, {} ch)",
                          m_halide_buffer.width(), m_halide_buffer.height(), m_halide_buffer.channels());
        }
    } else {
        spdlog::debug("WorkingImageCPU_Halide: Constructed with no initial image or invalid image data");
    }
}

std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError>
WorkingImageCPU_Halide::convertHalideToImageRegion()
{
    try {
        auto cpu_image_copy = std::make_unique<Common::ImageRegion>();

        // Set dimensions and format from the Halide buffer
        cpu_image_copy->m_width = static_cast<int>(m_halide_buffer.width());
        cpu_image_copy->m_height = static_cast<int>(m_halide_buffer.height());
        cpu_image_copy->m_channels = static_cast<int>(m_halide_buffer.channels());
        cpu_image_copy->m_format = Common::PixelFormat::RGBA_F32; // Assuming F32 for now

        // Deep copy of data from Halide buffer (which points to m_data) to the new ImageRegion
        cpu_image_copy->m_data = m_data;

        if (!cpu_image_copy->isValid()) {
            return std::unexpected(ErrorHandling::CoreError::InvalidImageRegion);
        }

        spdlog::debug("WorkingImageCPU_Halide::convertHalideToImageRegion: Exported ImageRegion ({}x{}, {} ch)",
                      cpu_image_copy->m_width, cpu_image_copy->m_height, cpu_image_copy->m_channels);

        return cpu_image_copy;

    } catch (const std::bad_alloc& e) {
        spdlog::critical("WorkingImageCPU_Halide::convertHalideToImageRegion: Failed to allocate memory: {}", e.what());
        return std::unexpected(ErrorHandling::CoreError::AllocationFailed);
        } catch (const std::exception& e) {
        spdlog::critical("WorkingImageCPU_Halide::convertHalideToImageRegion: Exception: {}", e.what());
            return std::unexpected(ErrorHandling::CoreError::Unexpected);
    }
}


std::expected<void,  ErrorHandling::CoreError>
WorkingImageCPU_Halide::updateFromCPU(const Common::ImageRegion& cpu_image)
{
    if (!cpu_image.isValid()) {
        spdlog::warn("WorkingImageCPU_Halide::updateFromCPU: Input ImageRegion is invalid");
        return std::unexpected(ErrorHandling::CoreError::InvalidImageRegion);
    }

    try {
        // Standard copy assignment
        m_data = cpu_image.m_data;

        if (m_data.empty()) {
            spdlog::error("WorkingImageCPU_Halide::updateFromCPU: Data vector is empty after copy");
            return std::unexpected(ErrorHandling::CoreError::InvalidImageRegion);
        }

        spdlog::debug("WorkingImageCPU_Halide::updateFromCPU: Copied {} elements from ImageRegion to internal storage",
                      m_data.size());

        initializeHalide(cpu_image);

        spdlog::debug("WorkingImageCPU_Halide::updateFromCPU: Created Halide::Buffer pointing to internal data ({}x{}, {} ch)",
                      m_halide_buffer.width(), m_halide_buffer.height(), m_halide_buffer.channels());

        return {}; // Success

    } catch (const std::bad_alloc& e) {
        spdlog::critical("WorkingImageCPU_Halide::updateFromCPU: Allocation failed: {}", e.what());
        return std::unexpected(ErrorHandling::CoreError::AllocationFailed);
    } catch (const std::exception& e) {
        spdlog::critical("WorkingImageCPU_Halide::updateFromCPU: Exception: {}", e.what());
        return std::unexpected(ErrorHandling::CoreError::InvalidImageRegion);
    }
}

std::expected<void, ErrorHandling::CoreError> WorkingImageCPU_Halide::updateFromCPU(Common::ImageRegion&& cpu_image)
{
    if (!cpu_image.isValid()) {
        spdlog::warn("WorkingImageCPU_Halide::updateFromCPU: Input ImageRegion is invalid");
        return std::unexpected(ErrorHandling::CoreError::InvalidImageRegion);
    }

    try {
        // PERFORMANCE: Steal the data pointer from the source.
        m_data = std::move(cpu_image.m_data);

        if (m_data.empty()) {
            spdlog::error("WorkingImageCPU_Halide::updateFromCPU: Data vector is empty after move");
            return std::unexpected(ErrorHandling::CoreError::InvalidImageRegion);
        }

        spdlog::debug("WorkingImageCPU_Halide::updateFromCPU: MOVED {} elements from ImageRegion to internal storage",
                      m_data.size());

        // Initialize Halide view to point to the now-owned data
        initializeHalide(cpu_image);

        spdlog::debug("WorkingImageCPU_Halide::updateFromCPU: Created Halide::Buffer pointing to internal data ({}x{}, {} ch)",
                      m_halide_buffer.width(), m_halide_buffer.height(), m_halide_buffer.channels());

        return {}; // Success

    } catch (const std::bad_alloc& e) {
        // Note: Move assignment usually doesn't throw bad_alloc, but initializeHalide or resize might.
        spdlog::critical("WorkingImageCPU_Halide::updateFromCPU: Allocation failed during init: {}", e.what());
        return std::unexpected(ErrorHandling::CoreError::AllocationFailed);
    } catch (const std::exception& e) {
        spdlog::critical("WorkingImageCPU_Halide::updateFromCPU: Exception: {}", e.what());
        return std::unexpected(ErrorHandling::CoreError::InvalidImageRegion);
    }
}

std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError>
WorkingImageCPU_Halide::exportToCPUCopy()
{
    if (!isValid()) {
        spdlog::warn("WorkingImageCPU_Halide::exportToCPUCopy: Current Halide buffer is invalid, cannot export");
        return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
    }

    return convertHalideToImageRegion();
}

std::pair<size_t, size_t> WorkingImageCPU_Halide::getSize() const
{
    if (!isValid()) {
        return {0, 0};
    }
    return {static_cast<size_t>(m_halide_buffer.width()), static_cast<size_t>(m_halide_buffer.height())};
}

size_t WorkingImageCPU_Halide::getChannels() const
{
    if (!isValid()) {
        return 0;
    }
    return static_cast<size_t>(m_halide_buffer.channels());
}

size_t WorkingImageCPU_Halide::getPixelCount() const
{
    if (!isValid()) {
        return 0;
    }
    return static_cast<size_t>(m_halide_buffer.width()) * m_halide_buffer.height();
}

size_t WorkingImageCPU_Halide::getDataSize() const
{
    if (!isValid()) {
        return 0;
    }
    return static_cast<size_t>(m_halide_buffer.size_in_bytes() / sizeof(float));
}

} // namespace CaptureMoment::Core::ImageProcessing
