/**
 * @file base_image_item.cpp
 * @brief Implementation of BaseImageItem
 * @author CaptureMoment Team
 * @date 2025
 */

#include "rendering/base_image_item.h"
#include <spdlog/spdlog.h>

namespace CaptureMoment::UI::Rendering {

// Constructor: Initializes the base item.
BaseImageItem::BaseImageItem()
{
    spdlog::debug("BaseImageItem: Created");
}

bool BaseImageItem::isImageValid() const
{
    QMutexLocker lock(&m_image_mutex);
    return m_full_image && m_full_image->isValid();
}

// Gets the width of the image.
int BaseImageItem::imageWidth() const
{
    QMutexLocker lock(&m_image_mutex);
    return m_image_width;
}

// Gets the height of the image.
int BaseImageItem::imageHeight() const
{
    QMutexLocker lock(&m_image_mutex);
    return m_image_height;
}

} // namespace CaptureMoment::UI::Rendering
