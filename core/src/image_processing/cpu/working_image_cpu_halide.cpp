/**
 * @file working_image_cpu_halide.cpp
 * @brief Implementation of WorkingImageCPU_Halide
 * @author CaptureMoment Team
 * @date 2026
 */

#include "image_processing/cpu/working_image_cpu_halide.h"
#include <spdlog/spdlog.h>
#include <memory>
#include <cstring>


namespace CaptureMoment::Core::ImageProcessing {

WorkingImageCPU_Halide::WorkingImageCPU_Halide(std::shared_ptr<Common::ImageRegion> initial_image)
    : m_halide_buffer() // Default constructor creates an empty buffer
{
    if (initial_image && initial_image->isValid()) {
        if (!updateFromCPU(*initial_image)) {
            spdlog::error("WorkingImageCPU_Halide: Failed to initialize Halide buffer from provided ImageRegion.");
        } else {
            spdlog::debug("WorkingImageCPU_Halide: Constructed and initialized with valid initial image ({}x{}, {} ch)",
                          m_halide_buffer.width(), m_halide_buffer.height(), m_halide_buffer.channels());
        }
    } else {
        spdlog::debug("WorkingImageCPU_Halide: Constructed with no initial image or invalid image data");
    }
}

WorkingImageCPU_Halide::~WorkingImageCPU_Halide() {
    spdlog::debug("WorkingImageCPU_Halide: Destructor called");
}

bool WorkingImageCPU_Halide::updateFromCPU(const Common::ImageRegion& cpu_image)
{
    if (!cpu_image.isValid()) {
        spdlog::warn("WorkingImageCPU_Halide::updateFromCPU: Input ImageRegion is invalid");
        return false;
    }

    try {
        // Create a new Halide buffer with the correct shape
        m_halide_buffer = Halide::Buffer<float>(
            cpu_image.m_width,
            cpu_image.m_height,
            cpu_image.m_channels
        );

        // Copy data from the ImageRegion into the Halide buffer's host memory
        std::memcpy(
            m_halide_buffer.data(),
            cpu_image.m_data.data(),
            cpu_image.m_data.size() * sizeof(float)
        );

    } catch (const std::exception& e) {
        spdlog::critical("WorkingImageCPU_Halide::updateFromCPU: Exception during buffer creation or data copy: {}", e.what());
        return false;
    } catch (...) {
        spdlog::critical("WorkingImageCPU_Halide::updateFromCPU: Unknown exception during buffer creation or data copy.");
        return false;
    }

    spdlog::debug("WorkingImageCPU_Halide::updateFromCPU: Successfully updated Halide buffer ({}x{}, {} ch)",
                  m_halide_buffer.width(), m_halide_buffer.height(), m_halide_buffer.channels());

    return true;
}

std::shared_ptr<Common::ImageRegion> WorkingImageCPU_Halide::exportToCPUCopy()
{
    if (!isValid()) {
        spdlog::warn("WorkingImageCPU_Halide::exportToCPUCopy: Current Halide buffer is invalid, cannot export");
        return nullptr;
    }

    std::shared_ptr<Common::ImageRegion> cpu_image_copy;
    try {
        cpu_image_copy = std::make_shared<Common::ImageRegion>();
        if (!cpu_image_copy) {
            spdlog::critical("WorkingImageCPU_Halide::exportToCPUCopy: Failed to allocate memory for exported ImageRegion (copy).");
            return nullptr;
        }

        // Set dimensions and format
        cpu_image_copy->m_width = static_cast<int>(m_halide_buffer.width());
        cpu_image_copy->m_height = static_cast<int>(m_halide_buffer.height());
        cpu_image_copy->m_channels = static_cast<int>(m_halide_buffer.channels());

        // Resize the data vector
        cpu_image_copy->m_data.resize(m_halide_buffer.size_in_bytes() / sizeof(float));

        // Copy data from the Halide buffer's host memory
        std::memcpy(
            cpu_image_copy->m_data.data(),
            m_halide_buffer.data(),
            cpu_image_copy->m_data.size() * sizeof(float)
        );

    } catch (const std::bad_alloc& e) {
        spdlog::critical("WorkingImageCPU_Halide::exportToCPUCopy: Failed to allocate memory: {}", e.what());
        return nullptr;
    } catch (const std::exception& e) {
        spdlog::critical("WorkingImageCPU_Halide::exportToCPUCopy: Exception during copy: {}", e.what());
        return nullptr;
    } catch (...) {
        spdlog::critical("WorkingImageCPU_Halide::exportToCPUCopy: Unknown exception during copy.");
        return nullptr;
    }

    if (!cpu_image_copy->isValid()) {
        spdlog::error("WorkingImageCPU_Halide::exportToCPUCopy: Exported ImageRegion copy is invalid.");
        return nullptr;
    }

    spdlog::debug("WorkingImageCPU_Halide::exportToCPUCopy: Successfully exported image data COPY ({}x{}, {} ch)",
                  cpu_image_copy->m_width, cpu_image_copy->m_height, cpu_image_copy->m_channels);

    return cpu_image_copy;
}

std::shared_ptr<Common::ImageRegion> WorkingImageCPU_Halide::exportToCPUShared() const
{
    spdlog::warn("WorkingImageCPU_Halide::exportToCPUShared: Cannot share Halide buffer data as ImageRegion without copying. Use exportToCPUCopy().");
    return nullptr;
}

std::pair<size_t, size_t> WorkingImageCPU_Halide::getSize() const
{
    if (!isValid()) {
        spdlog::warn("WorkingImageCPU_Halide::getSize: Halide buffer is invalid, returning {0, 0}");
        return {0, 0};
    }
    return {static_cast<size_t>(m_halide_buffer.width()), static_cast<size_t>(m_halide_buffer.height())};
}

size_t WorkingImageCPU_Halide::getChannels() const
{
    if (!isValid()) {
        spdlog::warn("WorkingImageCPU_Halide::getChannels: Halide buffer is invalid, returning 0");
        return 0;
    }
    return static_cast<size_t>(m_halide_buffer.channels());
}

size_t WorkingImageCPU_Halide::getPixelCount() const
{
    if (!isValid()) {
        spdlog::warn("WorkingImageCPU_Halide::getPixelCount: Halide buffer is invalid, returning 0");
        return 0;
    }
    return static_cast<size_t>(m_halide_buffer.width()) * m_halide_buffer.height();
}

size_t WorkingImageCPU_Halide::getDataSize() const
{
    if (!isValid()) {
        spdlog::warn("WorkingImageCPU_Halide::getDataSize: Halide buffer is invalid, returning 0");
        return 0;
    }
    return static_cast<size_t>(m_halide_buffer.size_in_bytes() / sizeof(float));
}

bool WorkingImageCPU_Halide::isValid() const
{
    return m_halide_buffer.defined();
}

Common::MemoryType WorkingImageCPU_Halide::getMemoryType() const
{
    return Common::MemoryType::CPU_RAM;
}

} // namespace CaptureMoment::Core::ImageProcessing
