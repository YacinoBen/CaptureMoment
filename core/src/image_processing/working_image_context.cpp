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
#include "image_processing/common/image_view.h"
#include <spdlog/spdlog.h>
#include <expected>

namespace CaptureMoment::Core::ImageProcessing {

bool WorkingImageContext::prepare(std::unique_ptr<Common::ImageRegion> original_tile)
{
    if (!original_tile) {
        return false;
    }

    // 1. Always recreate the Data Owner to ensure memory addresses are stable
    // (Prevents dangling pointers if std::vector reallocates internally)
    try {
        m_image_data = std::make_unique<WorkingImageData>(std::move(original_tile));
    } catch (const std::exception& e) {
        spdlog::error("[WorkingImageContext::prepare]: Exception creating data: {}", e.what());
        return false;
    }

    if (!m_image_data->isValid()) {
        spdlog::error("[WorkingImageContext::prepare]: Data is invalid after initialization.");
        return false;
    }

    // 2. Create the Hardware Worker
    auto new_hardware { WorkingImageFactory::create() };

    if (!new_hardware) {
        spdlog::error("[WorkingImageContext::prepare]: Failed to create Hardware backend.");
        return false;
    }

    // 3. Construct the view from our safely owned data
    ImageView view_data_image {
        .working_data = m_image_data->getWorkingDataSpan(),
        .original_data = m_image_data->getOriginalDataSpan(),
        .width = m_image_data->getWidth(),
        .height = m_image_data->getHeight(),
        .channels = m_image_data->getChannels()
    };

    // 4. Bind the view to the hardware worker
    if (!new_hardware->bindView(view_data_image)) {
        spdlog::error("[WorkingImageContext::prepare]: Failed to bind data to hardware.");
        return false;
    }

    m_working_image = std::move(new_hardware);
    spdlog::debug("[WorkingImageContext::prepare]: Created and bound new WorkingImage.");
    return true;
}


void WorkingImageContext::resetToOriginal()
{
    if (!m_image_data || !m_image_data->isValid()) {
        spdlog::warn("[WorkingImageContext::resetToOriginal]: No valid data to reset.");
        return;
    }

    // We reset the data at the Context level.
    // Because the Hardware works on std::span (pointers), the change is instantaneous
    // for the Hardware worker. No need to call a reset method on m_working_image!
    m_image_data->restoreOriginalData();
    spdlog::debug("[WorkingImageContext::resetToOriginal]: Data reset to original.");
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
WorkingImageContext::getDownsampled(Common::ImageDim target_width, Common::ImageDim target_height)
{
    if (!m_working_image) {
        return std::unexpected(ErrorHandling::CoreError::InvalidWorkingImage);
    }

    return m_working_image->downsample(target_width, target_height);
}

void WorkingImageContext::release() noexcept
{
    m_working_image.reset();
    m_image_data.reset();
    spdlog::debug("[WorkingImageContext::release]: Released working image and data.");
}

} // namespace CaptureMoment::Core::ImageProcessing

