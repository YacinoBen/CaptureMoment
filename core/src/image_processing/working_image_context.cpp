/**
 * @file working_image_context.cpp
 * @brief Implementation of working image context.
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#include "image_processing/working_image_context.h"
#include "common/error_handling/core_error.h"
#include "image_processing/factories/working_image_factory.h"

#include <spdlog/spdlog.h>
#include <expected>

namespace CaptureMoment::Core::ImageProcessing {

bool WorkingImageContext::prepare(std::unique_ptr<Common::ImageRegion>&& original_tile)
{
    if (!original_tile) {
        return false;
    }

    auto new_image = WorkingImageFactory::create(std::move(*original_tile));

    if (!new_image) {
        spdlog::error("[WorkingImageContext::prepare]: Failed to create WorkingImage.");
        return false;
    }

    m_working_image = std::move(new_image);
    spdlog::debug("[WorkingImageContext::prepare]: Created new WorkingImage.");
    return true;
}

bool WorkingImageContext::update(const Common::ImageRegion& original_tile)
{
    if (!m_working_image) {
        spdlog::error("[WorkingImageContext::update]: No WorkingImage to update.");
        return false;
    }

    auto result = m_working_image->updateFromCPU(original_tile);

    if (!result) {
        spdlog::warn("[WorkingImageContext::update]: updateFromCPU failed.");
        return false;
    }

    spdlog::debug("[WorkingImageContext::update]: WorkingImage updated.");
    return true;
}

std::shared_ptr<IWorkingImageHardware> 
WorkingImageContext::getWorkingImage() noexcept
{ 
    return m_working_image;
}

bool WorkingImageContext::isReady() const noexcept
{
    return m_working_image && m_working_image->isValid();
}

std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError> 
WorkingImageContext::getWorkingImageAsRegion() const
{
    if (!m_working_image) {
        return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
    }
    return m_working_image->exportToCPUCopy();
}

std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError>
WorkingImageContext::downsample(Common::ImageDim target_width, Common::ImageDim target_height)
{
    if (!m_working_image) {
        return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
    }

    return m_working_image->downsample(target_width, target_height);
}

void WorkingImageContext::release() noexcept
{
    m_working_image.reset();
}

} // namespace CaptureMoment::Core::ImageProcessing

