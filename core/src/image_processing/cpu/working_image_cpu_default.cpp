/**
 * @file working_image_cpu_default.cpp
 * @brief Implementation of WorkingImageCPU_Default.
 * @author CaptureMoment Team
 * @date 2026
 */

#include "image_processing/cpu/working_image_cpu_default.h"
#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::ImageProcessing {

WorkingImageCPU_Default::WorkingImageCPU_Default(std::unique_ptr<Common::ImageRegion> initial_image)
{
    // Transfer ownership of the input unique_ptr to our internal shared_ptr member.
    // This effectively converts unique ownership to shared ownership within the class.
    m_image_data = std::move(initial_image);

    if (m_image_data && m_image_data->isValid())
    {
        spdlog::debug("[WorkingImageCPU_Default] Constructed with valid initial image ({}x{}, {} ch)",
                      m_image_data->m_width, m_image_data->m_height, m_image_data->m_channels);
    }
    else
    {
        spdlog::debug("[WorkingImageCPU_Default] Constructed with no initial image or invalid image data");
    }
}

std::expected<void, std::error_code>
WorkingImageCPU_Default::updateFromCPU(const Common::ImageRegion& cpu_image)
{
    if (!cpu_image.isValid())
    {
        spdlog::warn("[WorkingImageCPU_Default] Input ImageRegion is invalid");
        return std::unexpected(ErrorHandling::make_error_code(ErrorHandling::CoreError::InvalidImageRegion));
    }

    std::shared_ptr<Common::ImageRegion> new_image_ptr;
    try
    {
        // Create a new ImageRegion to hold the copy
        new_image_ptr = std::make_shared<Common::ImageRegion>();
    }
    catch (const std::bad_alloc& e)
    {
        spdlog::critical("[WorkingImageCPU_Default] Failed to allocate memory for new ImageRegion: {}", e.what());
        return std::unexpected(ErrorHandling::make_error_code(ErrorHandling::CoreError::AllocationFailed));
    }

    if (!new_image_ptr)
    {
        spdlog::critical("[WorkingImageCPU_Default] std::make_shared returned a null pointer unexpectedly.");
        return std::unexpected(ErrorHandling::make_error_code(ErrorHandling::CoreError::AllocationFailed));
    }

    try
    {
        // Perform deep copy of data
        *new_image_ptr = cpu_image;
    }
    catch (...)
    {
        spdlog::critical("[WorkingImageCPU_Default] Exception occurred during assignment of ImageRegion data.");
        return std::unexpected(ErrorHandling::make_error_code(ErrorHandling::CoreError::InvalidImageRegion));
    }

    // Update internal state
    m_image_data = std::move(new_image_ptr);

    spdlog::debug("[WorkingImageCPU_Default] Successfully updated image data ({}x{}, {} ch)",
                  m_image_data->m_width, m_image_data->m_height, m_image_data->m_channels);

    return {}; // Success
}

std::expected<std::unique_ptr<Common::ImageRegion>, std::error_code>
WorkingImageCPU_Default::exportToCPUCopy()
{
    if (!isValid())
    {
        spdlog::warn("[WorkingImageCPU_Default] Current image data is invalid, cannot export");
        return std::unexpected(ErrorHandling::make_error_code(ErrorHandling::CoreError::InvalidWorkingImage));
    }

    std::unique_ptr<Common::ImageRegion> new_image_copy;
    try
    {
        new_image_copy = std::make_unique<Common::ImageRegion>();
        if (!new_image_copy)
        {
            spdlog::critical("[WorkingImageCPU_Default] Failed to allocate memory for exported ImageRegion (copy).");
            return std::unexpected(ErrorHandling::make_error_code(ErrorHandling::CoreError::AllocationFailed));
        }

        // Perform deep copy
        *new_image_copy = *m_image_data;
    }
    catch (const std::bad_alloc& e)
    {
        spdlog::critical("[WorkingImageCPU_Default] Failed to allocate memory for exported ImageRegion (copy): {}", e.what());
        return std::unexpected(ErrorHandling::make_error_code(ErrorHandling::CoreError::AllocationFailed));
    }
    catch (...)
    {
        spdlog::critical("[WorkingImageCPU_Default] Exception occurred during copy of ImageRegion data.");
        return std::unexpected(ErrorHandling::make_error_code(ErrorHandling::CoreError::InvalidImageRegion));
    }

    if (!new_image_copy->isValid())
    {
        spdlog::error("[WorkingImageCPU_Default] Exported ImageRegion copy is invalid (unexpected).");
        return std::unexpected(ErrorHandling::make_error_code(ErrorHandling::CoreError::InvalidImageRegion));
    }

    spdlog::debug("[WorkingImageCPU_Default] Successfully exported image data COPY ({}x{}, {} ch)",
                  new_image_copy->m_width, new_image_copy->m_height, new_image_copy->m_channels);

    return new_image_copy;
}

std::expected<std::shared_ptr<Common::ImageRegion>, std::error_code>
WorkingImageCPU_Default::exportToCPUShared() const
{
    if (!isValid())
    {
        spdlog::warn("[WorkingImageCPU_Default] Current image data is invalid, cannot export shared reference");
        return std::unexpected(ErrorHandling::make_error_code(ErrorHandling::CoreError::InvalidWorkingImage));
    }

    // No copy, just return the internal shared pointer
    std::shared_ptr<Common::ImageRegion> shared_ref = m_image_data;

    spdlog::debug("[WorkingImageCPU_Default] Successfully exported shared reference to image data ({}x{}, {} ch)",
                  shared_ref->m_width, shared_ref->m_height, shared_ref->m_channels);

    return shared_ref;
}

std::pair<size_t, size_t> WorkingImageCPU_Default::getSize() const
{
    if (!isValid())
    {
        return {0, 0};
    }
    return {static_cast<size_t>(m_image_data->m_width), static_cast<size_t>(m_image_data->m_height)};
}

size_t WorkingImageCPU_Default::getChannels() const
{
    if (!isValid())
    {
        return 0;
    }
    return static_cast<size_t>(m_image_data->m_channels);
}

size_t WorkingImageCPU_Default::getPixelCount() const
{
    if (!isValid())
    {
        return 0;
    }
    return static_cast<size_t>(m_image_data->m_width) * m_image_data->m_height;
}

size_t WorkingImageCPU_Default::getDataSize() const
{
    if (!isValid())
    {
        return 0;
    }
    return static_cast<size_t>(m_image_data->m_width) * m_image_data->m_height * m_image_data->m_channels;
}

} // namespace CaptureMoment::Core::ImageProcessing
