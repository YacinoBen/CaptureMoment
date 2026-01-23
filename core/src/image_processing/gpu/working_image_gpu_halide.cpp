/**
 * @file working_image_gpu_halide.cpp
 * @brief Implementation of WorkingImageGPU_Halide
 * @author CaptureMoment Team
 * @date 2026
 */

#include "image_processing/gpu/working_image_gpu_halide.h"
#include <spdlog/spdlog.h>
#include <memory>
#include <cstring>

#include "HalideRuntime.h"

namespace CaptureMoment::Core::ImageProcessing {

WorkingImageGPU_Halide::WorkingImageGPU_Halide(std::shared_ptr<Common::ImageRegion> initial_image)
    : m_cached_width(0), m_cached_height(0), m_cached_channels(0), m_metadata_valid(false)
{
    if (initial_image && initial_image->isValid()) {
        if (!updateFromCPU(*initial_image)) {
            spdlog::error("WorkingImageGPU_Halide: Failed to initialize GPU buffer from provided ImageRegion.");
        } else {
            spdlog::debug("WorkingImageGPU_Halide: Constructed and initialized with valid initial image ({}x{}, {} ch)",
                          m_cached_width, m_cached_height, m_cached_channels);
        }
    } else {
        spdlog::debug("WorkingImageGPU_Halide: Constructed with no initial image or invalid image data");
    }
}

WorkingImageGPU_Halide::~WorkingImageGPU_Halide() {
    spdlog::debug("WorkingImageGPU_Halide: Destructor called");
}

bool WorkingImageGPU_Halide::updateFromCPU(const Common::ImageRegion &cpu_image)
{
    if (!cpu_image.isValid()) {
        spdlog::warn("WorkingImageGPU_Halide::updateFromCPU: Input ImageRegion is invalid");
        return false;
    }

    Halide::Target gpu_target = Halide::get_host_target();
    if (!gpu_target.has_gpu_feature()) {
        spdlog::error("WorkingImageGPU_Halide::updateFromCPU: No valid GPU target available.");
        return false;
    }

    try {
        // Copy data from ImageRegion to internal storage
        m_data = cpu_image.m_data;

        if (m_data.empty()) {
            spdlog::error("WorkingImageGPU_Halide::updateFromCPU: Data vector is empty after copy");
            return false;
        }

        spdlog::debug("WorkingImageGPU_Halide::updateFromCPU: Copied {} elements from ImageRegion to internal storage",
                      m_data.size());

        // Initialize the Halide buffer to point to our internal data
        initializeHalide(cpu_image);

        // Mark host data as dirty and copy to the GPU device
        m_halide_buffer.set_host_dirty();
        int result = m_halide_buffer.copy_to_device(gpu_target);
        if (result != 0) {
            spdlog::critical("WorkingImageGPU_Halide::updateFromCPU: copy_to_device failed with error code: {}", result);
            return false;
        }

        // Cache metadata since querying the GPU buffer directly can be unreliable
        m_cached_width = cpu_image.m_width;
        m_cached_height = cpu_image.m_height;
        m_cached_channels = cpu_image.m_channels;
        m_metadata_valid = true;

    } catch (const std::exception& e) {
        spdlog::critical("WorkingImageGPU_Halide::updateFromCPU: Exception during GPU buffer setup or transfer: {}", e.what());
        return false;
    } catch (...) {
        spdlog::critical("WorkingImageGPU_Halide::updateFromCPU: Unknown exception during GPU buffer setup or transfer.");
        return false;
    }

    spdlog::debug("WorkingImageGPU_Halide::updateFromCPU: Successfully updated GPU image data ({}x{}, {} ch)",
                  m_cached_width, m_cached_height, m_cached_channels);

    return true;
}

std::shared_ptr<Common::ImageRegion> WorkingImageGPU_Halide::exportToCPUCopy()
{
    if (!isValid()) {
        spdlog::warn("WorkingImageGPU_Halide::exportToCPUCopy: Current GPU image data is invalid, cannot export");
        return nullptr;
    }

    Halide::Target gpu_target = Halide::get_host_target();
    if (!gpu_target.has_gpu_feature()) {
        spdlog::error("WorkingImageGPU_Halide::exportToCPUCopy: No valid GPU target available.");
        return nullptr;
    }

    std::shared_ptr<Common::ImageRegion> cpu_image_copy;
    try {
        cpu_image_copy = std::make_shared<Common::ImageRegion>();
        if (!cpu_image_copy) {
            spdlog::critical("WorkingImageGPU_Halide::exportToCPUCopy: Failed to allocate memory for exported ImageRegion (copy).");
            return nullptr;
        }

        // Use cached metadata
        cpu_image_copy->m_width = static_cast<int>(m_cached_width);
        cpu_image_copy->m_height = static_cast<int>(m_cached_height);
        cpu_image_copy->m_channels = static_cast<int>(m_cached_channels);
        cpu_image_copy->m_data.resize(m_cached_width * m_cached_height * m_cached_channels);

        // Sync data from GPU device back to host memory
        int result = m_halide_buffer.copy_to_host();
        if (result != 0) {
            spdlog::critical("WorkingImageGPU_Halide::exportToCPUCopy: copy_to_host failed with error code: {}", result);
            return nullptr;
        }

        // Now copy from the synced host memory
        std::memcpy(
            cpu_image_copy->m_data.data(),
            m_halide_buffer.data(),
            cpu_image_copy->m_data.size() * sizeof(float)
        );

    } catch (const std::bad_alloc& e) {
        spdlog::critical("WorkingImageGPU_Halide::exportToCPUCopy: Failed to allocate memory: {}", e.what());
        return nullptr;
    } catch (const std::exception& e) {
        spdlog::critical("WorkingImageGPU_Halide::exportToCPUCopy: Exception during GPU-to-host transfer: {}", e.what());
        return nullptr;
    } catch (...) {
        spdlog::critical("WorkingImageGPU_Halide::exportToCPUCopy: Unknown exception during GPU-to-host transfer.");
        return nullptr;
    }

    if (!cpu_image_copy->isValid()) {
        spdlog::error("WorkingImageGPU_Halide::exportToCPUCopy: Exported ImageRegion copy is invalid.");
        return nullptr;
    }

    spdlog::debug("WorkingImageGPU_Halide::exportToCPUCopy: Successfully exported image data COPY ({}x{}, {} ch)",
                  cpu_image_copy->m_width, cpu_image_copy->m_height, cpu_image_copy->m_channels);

    return cpu_image_copy;
}

std::pair<size_t, size_t> WorkingImageGPU_Halide::getSize() const
{
    if (!isValid()) {
        spdlog::warn("WorkingImageGPU_Halide::getSize: Image data is invalid, returning {0, 0}");
        return {0, 0};
    }
    if (!m_metadata_valid) {
        spdlog::error("WorkingImageGPU_Halide::getSize: Metadata cache is invalid.");
        return {0, 0};
    }
    return {m_cached_width, m_cached_height};
}

size_t WorkingImageGPU_Halide::getChannels() const
{
    if (!isValid()) {
        spdlog::warn("WorkingImageGPU_Halide::getChannels: Image data is invalid, returning 0");
        return 0;
    }
    if (!m_metadata_valid) {
        spdlog::error("WorkingImageGPU_Halide::getChannels: Metadata cache is invalid.");
        return 0;
    }
    return m_cached_channels;
}

size_t WorkingImageGPU_Halide::getPixelCount() const
{
    if (!isValid()) {
        spdlog::warn("WorkingImageGPU_Halide::getPixelCount: Image data is invalid, returning 0");
        return 0;
    }
    if (!m_metadata_valid) {
        spdlog::error("WorkingImageGPU_Halide::getPixelCount: Metadata cache is invalid.");
        return 0;
    }
    return m_cached_width * m_cached_height;
}

size_t WorkingImageGPU_Halide::getDataSize() const
{
    if (!isValid()) {
        spdlog::warn("WorkingImageGPU_Halide::getDataSize: Image data is invalid, returning 0");
        return 0;
    }
    if (!m_metadata_valid) {
        spdlog::error("WorkingImageGPU_Halide::getDataSize: Metadata cache is invalid.");
        return 0;
    }
    return m_cached_width * m_cached_height * m_cached_channels;
}

bool WorkingImageGPU_Halide::updateCachedMetadata() const {
    // In this implementation, metadata is cached during updateFromCPU.
    // This function is kept for interface completeness but is not used in getters.
    if (m_halide_buffer.defined() && m_cached_width > 0 && m_cached_height > 0 && m_cached_channels > 0) {
        m_metadata_valid = true;
        return true;
    }
    m_metadata_valid = false;
    return false;
}

} // namespace CaptureMoment::Core::ImageProcessing
