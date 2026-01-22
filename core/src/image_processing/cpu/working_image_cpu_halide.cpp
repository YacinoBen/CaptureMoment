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

std::shared_ptr<Common::ImageRegion> WorkingImageCPU_Halide::convertHalideToImageRegion()
{
    std::shared_ptr<Common::ImageRegion> cpu_image_copy;

    try {
        cpu_image_copy = std::make_shared<Common::ImageRegion>();
        if (!cpu_image_copy) {
            spdlog::critical("WorkingImageCPU_Halide::convertHalideToImageRegion: Failed to allocate memory for exported ImageRegion (copy).");
            return nullptr;
        }

        // Set dimensions and format
        cpu_image_copy->m_width = static_cast<int>(m_halide_buffer.width());
        cpu_image_copy->m_height = static_cast<int>(m_halide_buffer.height());
        cpu_image_copy->m_channels = static_cast<int>(m_halide_buffer.channels());

        cpu_image_copy->m_data = m_data;


        if (!cpu_image_copy->isValid()) {
            spdlog::error("WorkingImageCPU_Halide::convertHalideToImageRegion: Exported ImageRegion is invalid.");
            return nullptr;
        }

        spdlog::debug("WorkingImageCPU_Halide::convertHalideToImageRegion: Exported ImageRegion ({}x{}, {} ch)",
                      cpu_image_copy->m_width, cpu_image_copy->m_height, cpu_image_copy->m_channels);

        return cpu_image_copy;

    } catch (const std::bad_alloc& e) {
        spdlog::critical("WorkingImageCPU_Halide::convertHalideToImageRegion: Failed to allocate memory: {}", e.what());
        return nullptr;
    } catch (const std::exception& e) {
        spdlog::critical("WorkingImageCPU_Halide::convertHalideToImageRegion: Exception: {}", e.what());
        return nullptr;
    }

    return cpu_image_copy;
}


void WorkingImageCPU_Halide::initializeHalide(const Common::ImageRegion& cpu_image)
{
    m_halide_buffer = Halide::Buffer<float>(
        m_data.data(),
        cpu_image.m_width,
        cpu_image.m_height,
        cpu_image.m_channels
        );
}

bool WorkingImageCPU_Halide::updateFromCPU(const Common::ImageRegion &cpu_image)
{
    if (!cpu_image.isValid()) {
        spdlog::warn("WorkingImageCPU_Halide::updateFromCPU: Input ImageRegion is invalid");
        return false;
    }

    try {

        m_data = cpu_image.m_data;

        if (m_data.empty()) {
            spdlog::error("WorkingImageCPU_Halide::updateFromCPU: Data vector is empty after copy");
            return false;
        }

        spdlog::debug("WorkingImageCPU_Halide::updateFromCPU: Copied {} elements from ImageRegion to internal storage",
                      m_data.size());


        initializeHalide(cpu_image);

        spdlog::debug("WorkingImageCPU_Halide::updateFromCPU: Created Halide::Buffer pointing to internal data ({}x{}, {} ch)",
                      m_halide_buffer.width(), m_halide_buffer.height(), m_halide_buffer.channels());

        return true;

    } catch (const std::exception& e) {
        spdlog::critical("WorkingImageCPU_Halide::updateFromCPU: Exception: {}", e.what());
        return false;
    }
}

std::shared_ptr<Common::ImageRegion> WorkingImageCPU_Halide::exportToCPUCopy()
{
    if (!isValid()) {
        spdlog::warn("WorkingImageCPU_Halide::exportToCPUCopy: Current Halide buffer is invalid, cannot export");
        return nullptr;
    }

    return convertHalideToImageRegion();
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

} // namespace CaptureMoment::Core::ImageProcessing
