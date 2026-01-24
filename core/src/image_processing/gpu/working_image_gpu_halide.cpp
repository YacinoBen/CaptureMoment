/**
 * @file working_image_gpu_halide.cpp
 * @brief Implementation of WorkingImageGPU_Halide.
 * @details Relies on AppConfig to provide the active Halide Target.
 *          Responsibility is strictly execution, not device selection.
 * @author CaptureMoment Team
 * @date 2026
 */

#include "image_processing/gpu/working_image_gpu_halide.h"
#include "config/app_config.h"
#include <spdlog/spdlog.h>
#include <memory>
#include <utility>
#include <cstring>

#include "HalideRuntime.h"

namespace CaptureMoment::Core::ImageProcessing {


WorkingImageGPU_Halide::WorkingImageGPU_Halide(std::unique_ptr<Common::ImageRegion> initial_image)
    : m_cached_width(0), m_cached_height(0), m_cached_channels(0), m_metadata_valid(false)
{
    // We assume AppConfig::getHalideTarget() has been correctly initialized
    // by IBackendDecider at application startup.

    if (initial_image && initial_image->isValid())
    {
        auto result = updateFromCPU(std::move(*initial_image));
        if (!result)
        {
            spdlog::error("[WorkingImageGPU_Halide] Failed to initialize GPU buffer. Reason: {}", result.error().message());
        }
        else
        {
            spdlog::debug("[WorkingImageGPU_Halide] Constructed and initialized ({}x{}, {} ch)",
                          m_cached_width, m_cached_height, m_cached_channels);
        }
    }
    else
    {
        spdlog::debug("[WorkingImageGPU_Halide] Constructed with no initial image or invalid data");
    }
}


std::expected<void, std::error_code>
WorkingImageGPU_Halide::updateFromCPU(const Common::ImageRegion& cpu_image)
{
    if (!cpu_image.isValid())
    {
        return std::unexpected(ErrorHandling::make_error_code(ErrorHandling::CoreError::InvalidImageRegion));
    }

    try
    {
        // Retrieve the configured Halide Target (e.g. CUDA, Metal, Vulkan)
        // This is where we use the result of IBackendDecider
        Halide::Target target = Config::AppConfig::getHalideTarget();

        // Copy data from ImageRegion to internal storage
        m_data = cpu_image.m_data;

        if (m_data.empty())
        {
            return std::unexpected(ErrorHandling::make_error_code(ErrorHandling::CoreError::AllocationFailed));
        }

        spdlog::debug("[WorkingImageGPU_Halide] Copied {} elements to internal storage", m_data.size());

        // Initialize Halide buffer (CPU view)
        initializeHalide(cpu_image);

        // Mark host data as dirty and copy to GPU device
        m_halide_buffer.set_host_dirty();
        int result = m_halide_buffer.copy_to_device(target);
        if (result != 0)
        {
            spdlog::critical("[WorkingImageGPU_Halide] copy_to_device failed with error code: {}", result);
            return std::unexpected(ErrorHandling::make_error_code(ErrorHandling::CoreError::InvalidWorkingImage));
        }

        updateCachedMetadata(cpu_image);
        return {}; // Success

    }
    catch (const std::bad_alloc& e)
    {
        spdlog::critical("[WorkingImageGPU_Halide] Allocation failed: {}", e.what());
        return std::unexpected(ErrorHandling::make_error_code(ErrorHandling::CoreError::AllocationFailed));
    }
    catch (const std::exception& e)
    {
        spdlog::critical("[WorkingImageGPU_Halide] Exception: {}", e.what());
        return std::unexpected(ErrorHandling::make_error_code(ErrorHandling::CoreError::InvalidImageRegion));
    }
}

std::expected<void, std::error_code>
WorkingImageGPU_Halide::updateFromCPU(Common::ImageRegion&& cpu_image)
{
    if (!cpu_image.isValid())
    {
        return std::unexpected(ErrorHandling::make_error_code(ErrorHandling::CoreError::InvalidImageRegion));
    }

    try
    {
        // Use the configured target provided by IBackendDecider
        Halide::Target target = Config::AppConfig::getHalideTarget();

        // PERFORMANCE: Move data pointer (no CPU copy)
        m_data = std::move(cpu_image.m_data);

        if (m_data.empty())
        {
            return std::unexpected(ErrorHandling::make_error_code(ErrorHandling::CoreError::AllocationFailed));
        }

        spdlog::debug("[WorkingImageGPU_Halide] MOVED {} elements to internal storage", m_data.size());

        // Initialize Halide buffer (CPU view)
        initializeHalide(cpu_image);

        // Transfer to GPU
        m_halide_buffer.set_host_dirty();
        int result = m_halide_buffer.copy_to_device(target);
        if (result != 0)
        {
            spdlog::critical("[WorkingImageGPU_Halide] copy_to_device failed with error code: {}", result);
            return std::unexpected(ErrorHandling::make_error_code(ErrorHandling::CoreError::InvalidWorkingImage));
        }

        updateCachedMetadata(cpu_image);
        return {}; // Success

    }
    catch (const std::bad_alloc& e)
    {
        spdlog::critical("[WorkingImageGPU_Halide] Allocation failed: {}", e.what());
        return std::unexpected(ErrorHandling::make_error_code(ErrorHandling::CoreError::AllocationFailed));
    }
    catch (const std::exception& e)
    {
        spdlog::critical("[WorkingImageGPU_Halide] Exception: {}", e.what());
        return std::unexpected(ErrorHandling::make_error_code(ErrorHandling::CoreError::InvalidImageRegion));
    }
}

std::expected<std::unique_ptr<Common::ImageRegion>, std::error_code>
WorkingImageGPU_Halide::exportToCPUCopy()
{
    if (!isValid())
    {
        return std::unexpected(ErrorHandling::make_error_code(ErrorHandling::CoreError::InvalidWorkingImage));
    }

    try
    {
        auto cpu_image_copy = std::make_unique<Common::ImageRegion>();

        // Set dimensions from cache
        cpu_image_copy->m_width = static_cast<int>(m_cached_width);
        cpu_image_copy->m_height = static_cast<int>(m_cached_height);
        cpu_image_copy->m_channels = static_cast<int>(m_cached_channels);
        cpu_image_copy->m_format = Common::PixelFormat::RGBA_F32;

        cpu_image_copy->m_data.resize(m_cached_width * m_cached_height * m_cached_channels);

        // Sync data from GPU device back to host memory
        // Note: copy_to_host() does NOT need a Target argument, unlike copy_to_device.
        int result = m_halide_buffer.copy_to_host();
        if (result != 0)
        {
            spdlog::critical("[WorkingImageGPU_Halide] copy_to_host failed with error code: {}", result);
            return std::unexpected(ErrorHandling::make_error_code(ErrorHandling::CoreError::InvalidWorkingImage));
        }

        // Copy from synced host memory (m_halide_buffer) to new ImageRegion vector
        std::memcpy(
            cpu_image_copy->m_data.data(),
            m_halide_buffer.data(),
            cpu_image_copy->m_data.size() * sizeof(float)
            );

        if (!cpu_image_copy->isValid())
        {
            return std::unexpected(ErrorHandling::make_error_code(ErrorHandling::CoreError::InvalidImageRegion));
        }

        spdlog::debug("[WorkingImageGPU_Halide] Successfully exported data COPY ({}x{}, {} ch)",
                      cpu_image_copy->m_width, cpu_image_copy->m_height, cpu_image_copy->m_channels);

        return cpu_image_copy;

    }
    catch (const std::bad_alloc& e)
    {
        spdlog::critical("[WorkingImageGPU_Halide] Failed to allocate memory: {}", e.what());
        return std::unexpected(ErrorHandling::make_error_code(ErrorHandling::CoreError::AllocationFailed));
    }
    catch (const std::exception& e)
    {
        spdlog::critical("[WorkingImageGPU_Halide] Exception: {}", e.what());
        return std::unexpected(ErrorHandling::make_error_code(ErrorHandling::CoreError::InvalidImageRegion));
    }
}

std::pair<size_t, size_t> WorkingImageGPU_Halide::getSize() const
{
    if (!isValid())
    {
        return {0, 0};
    }
    return {m_cached_width, m_cached_height};
}

size_t WorkingImageGPU_Halide::getChannels() const
{
    if (!isValid())
    {
        return 0;
    }
    return m_cached_channels;
}

size_t WorkingImageGPU_Halide::getPixelCount() const
{
    if (!isValid())
    {
        return 0;
    }
    return m_cached_width * m_cached_height;
}

size_t WorkingImageGPU_Halide::getDataSize() const
{
    if (!isValid())
    {
        return 0;
    }
    return m_cached_width * m_cached_height * m_cached_channels;
}

void WorkingImageGPU_Halide::updateCachedMetadata(const Common::ImageRegion& region)
{
    m_cached_width = region.m_width;
    m_cached_height = region.m_height;
    m_cached_channels = region.m_channels;
    m_metadata_valid = true;
}

} // namespace CaptureMoment::Core::ImageProcessing
