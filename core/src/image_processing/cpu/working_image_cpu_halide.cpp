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

        // Deep copy from unique_ptr back to ImageRegion's vector
        cpu_image_copy->m_data.assign(m_data.get(), m_data.get() + m_data_size);

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
        // Calculate required size
        size_t required_size = static_cast<size_t>(cpu_image.m_width) *
                               cpu_image.m_height *
                               cpu_image.m_channels;

        // Check allocation size and use No-Init allocation
        if (!m_data || m_data_size != required_size) {
            // make_unique_for_overwrite allocates memory WITHOUT zero-initialization.
            // This saves the time spent writing 0s to the buffer before copying real data.
            m_data = std::make_unique_for_overwrite<float[]>(required_size);
            m_data_size = required_size;

            spdlog::trace("WorkingImageCPU_Halide::updateFromCPU: Reallocated internal buffer to {} elements (No-Init).", required_size);
        }

        // Fast memory copy
        std::memcpy(m_data.get(), cpu_image.m_data.data(), required_size * sizeof(float));

        spdlog::debug("WorkingImageCPU_Halide::updateFromCPU: Copied {} elements from ImageRegion to internal storage",
                      required_size);

        initializeHalide(cpu_image);

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
        // Calculate required size
        size_t required_size = static_cast<size_t>(cpu_image.m_width) *
                               cpu_image.m_height *
                               cpu_image.m_channels;

        // we cannot steal the memory directly into a unique_ptr (different allocators/types).
        // We still benefit from the No-Init reallocation logic if sizes differ.
        if (!m_data || m_data_size != required_size) {
            m_data = std::make_unique_for_overwrite<float[]>(required_size);
            m_data_size = required_size;
        }

        // Copy data (Fastest way to move from vector to unique_ptr)
        std::memcpy(m_data.get(), cpu_image.m_data.data(), required_size * sizeof(float));

        spdlog::debug("WorkingImageCPU_Halide::updateFromCPU: Moved/Copied {} elements from ImageRegion to internal storage",
                      required_size);

        initializeHalide(cpu_image);

        return {}; // Success

    } catch (const std::bad_alloc& e) {
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

std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError>
WorkingImageGPU_Halide::exportToCPUMove()
{
    if (!isValid()) {
        spdlog::warn("[WorkingImageGPU_Halide] exportToCPUMove called on invalid image");
        return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
    }

    try {
        // Step 1: Sync GPU data to host memory
        // This is the unavoidable GPU→CPU transfer when using GPU memory.
        // The data ends up in m_halide_buffer (which points to m_data).
        int result = m_halide_buffer.copy_to_host();
        if (result != 0) {
            spdlog::critical("[WorkingImageGPU_Halide] copy_to_host failed in exportToCPUMove: {}", result);
            return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
        }

        // Step 2: Create vector and copy data
        // We create a new vector and copy into it - this is the single memcpy
        // that we cannot avoid when transferring from unique_ptr to vector.
        std::vector<float> moved_data(m_data_size);
        std::memcpy(moved_data.data(), m_data.get(), m_data_size * sizeof(float));

        // Step 3: Use move constructor for zero-copy ImageRegion creation
        // The vector is moved into the ImageRegion without copying.
        auto cpu_image = std::make_unique<Common::ImageRegion>(
            std::move(moved_data),
            static_cast<int>(m_cached_width),
            static_cast<int>(m_cached_height),
            static_cast<int>(m_cached_channels)
            );
        cpu_image->m_format = Common::PixelFormat::RGBA_F32;

        // Step 4: Invalidate this WorkingImage
        // Reset all state to prevent accidental reuse after data transfer.
        m_data.reset();
        m_data_size = 0;
        m_metadata_valid = false;
        m_cached_width = 0;
        m_cached_height = 0;
        m_cached_channels = 0;

        spdlog::debug("[WorkingImageGPU_Halide] Moved {} elements to ImageRegion (buffer invalidated)",
                      cpu_image->m_data.size());

        return cpu_image;
    }
    catch (const std::bad_alloc& e) {
        spdlog::critical("[WorkingImageGPU_Halide] Allocation failed in exportToCPUMove: {}", e.what());
        return std::unexpected(ErrorHandling::CoreError::AllocationFailed);
    }
    catch (const std::exception& e) {
        spdlog::critical("[WorkingImageGPU_Halide] Exception in exportToCPUMove: {}", e.what());
        return std::unexpected(ErrorHandling::CoreError::InvalidImageRegion);
    }
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
