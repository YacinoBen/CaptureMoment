/**
 * @file painted_image_item.cpp
 * @brief Implementation of PaintedImageItem using QQuickPaintedItem and QPainter
 * @author CaptureMoment Team
 * @date 2025
 */

#include "rendering/painted_image_item.h"
#include <spdlog/spdlog.h>
#include <QPainter>
#include <QMutexLocker>
#include <algorithm>
#include <cstring>

namespace CaptureMoment::UI::Rendering {

PaintedImageItem::PaintedImageItem(QQuickItem* parent)
    : QQuickPaintedItem(parent)
{
    spdlog::info("PaintedImageItem: Created");
}

// Sets the full image to be displayed.
// Updates the internal image data and schedules a repaint.
void PaintedImageItem::setImage(const std::shared_ptr<Core::Common::ImageRegion>& image)
{
    if (!image || !image->isValid()){
            spdlog::warn("PaintedImageItem::setImage: Invalid image region");
            return;
        }
        
    spdlog::info("PaintedImageItem::setImage: {}x{}", image->m_width, image->m_height);

    // Protect access to shared data (m_current_qimage, m_image_width, m_image_height)
    {
        QMutexLocker lock(&m_image_mutex);
        // Convert ImageRegion to QImage
        m_current_qimage = convertImageRegionToQImage(*image);
        m_image_width = image->m_width;
        m_image_height = image->m_height;

        spdlog::info("PaintedImageItem::setImage: FINAL QImage {}x{}",
                     m_current_qimage.width(), m_current_qimage.height());
        spdlog::info("PaintedImageItem::setImage: Pixel (0,0): {},{},{}",
                     m_current_qimage.pixelColor(0, 0).red(),
                     m_current_qimage.pixelColor(0, 0).green(),
                     m_current_qimage.pixelColor(0, 0).blue());
        spdlog::info("PaintedImageItem::setImage: Pixel (0, {}): {},{},{}",
                     m_current_qimage.height()-1,
                     m_current_qimage.pixelColor(0, m_current_qimage.height()-1).red(),
                     m_current_qimage.pixelColor(0, m_current_qimage.height()-1).green(),
                     m_current_qimage.pixelColor(0, m_current_qimage.height()-1).blue());
    }

    // Emit signal for QML binding
    emit imageSizeChanged();

    // Trigger a repaint to reflect the new image.
    update();
}

// Updates a specific tile of the displayed image.
// Merges the tile data into the internal QImage buffer and schedules a repaint.
void PaintedImageItem::updateTile(const std::shared_ptr<Core::Common::ImageRegion>& tile)
{
    if (!tile || !tile->isValid()) {
        spdlog::warn("PaintedImageItem::updateTile: Invalid tile");
        return;
    }

    if (!isImagePaintValid()) {
        spdlog::warn("PaintedImageItem::paint: No source image or no converted image to paint");
        return;
    }

    QMutexLocker lock(&m_image_mutex);

    spdlog::info("PaintedImageItem::updateTile: tile {}x{} at ({},{})",
                 tile->m_width, tile->m_height, tile->m_x, tile->m_y);

    // Full Tile
    if (tile->m_x == 0 && tile->m_y == 0 &&
        tile->m_width == m_current_qimage.width() &&
        tile->m_height == m_current_qimage.height()) {

        spdlog::info("PaintedImageItem::updateTile: Full image replacement");
        m_current_qimage = convertImageRegionToQImage(*tile);

    } else {
        // Partial Tile, use QPainter
        spdlog::info("PaintedImageItem::updateTile: Partial tile using QPainter");
        QImage tileImage = convertImageRegionToQImage(*tile);
        QPainter painter(&m_current_qimage);
        painter.drawImage(tile->m_x, tile->m_y, tileImage);
        painter.end();
    }

    spdlog::info("PaintedImageItem::updateTile: Pixel (0,0): {},{},{}",
                 m_current_qimage.scanLine(0)[0],
                 m_current_qimage.scanLine(0)[1],
                 m_current_qimage.scanLine(0)[2]);

    spdlog::info("PaintedImageItem::updateTile: Pixel (0, 528): {},{},{}",
                 m_current_qimage.scanLine(528)[0],
                 m_current_qimage.scanLine(528)[1],
                 m_current_qimage.scanLine(528)[2]);

    update();
}

// Paints the image using QPainter.
void PaintedImageItem::paint(QPainter* painter)
{

    if (!painter) {
        spdlog::error("PaintedImageItem::paint: QPainter is null");
        return;
    }

    if (!isImagePaintValid()) {
        spdlog::warn("PaintedImageItem::paint: No source image or no converted image to paint");
        return;
    }

    // Protect access to the shared image data during painting
    QMutexLocker lock(&m_image_mutex);

    spdlog::trace("PaintedImageItem::paint: About to draw image");
    spdlog::info("PaintedImageItem::paint: Drawing m_current_qimage width: {}, m_current_qimage height {}, sample pixel (0,0) RGB: {},{},{}",
                 m_current_qimage.width(),
                 m_current_qimage.height(),
                 m_current_qimage.pixelColor(0, 0).red(),
                 m_current_qimage.pixelColor(0, 0).green(),
                 m_current_qimage.pixelColor(0, 0).blue()
                 );

    // Save the current painter state
    painter->save();

    // Apply zoom and pan transformations
    painter->translate(m_pan);
    painter->scale(m_zoom, m_zoom);

    QPointF topLeft(0, 0); // Position abefore transformation
    QPointF topLeftTransformed = painter->transform().map(topLeft); // Position after transformation
    spdlog::info("PaintedImageItem::paint: About to draw image. Logical top-left: ({}, {}), Transformed top-left: ({}, {}), Image size: {}x{}, Zoom: {}, Pan: ({}, {})",
                 topLeft.x(), topLeft.y(),
                 topLeftTransformed.x(), topLeftTransformed.y(),
                 m_current_qimage.width(), m_current_qimage.height(),
                 m_zoom, m_pan.x(), m_pan.y());

    // Draw the image
    painter->drawImage(QPointF(0, 0), m_current_qimage);

    // Restore the painter state
    painter->restore();

    spdlog::trace("PaintedImageItem::paint: Frame painted");
}

    // Converts an ImageRegion to a QImage.
    QImage PaintedImageItem::convertImageRegionToQImage(const Core::Common::ImageRegion& region) const {
       if (!region.isValid()) {
        spdlog::error("PaintedImageItem::convertImageRegionToQImage: Invalid region");
        return QImage();
    }

    spdlog::info("convertImageRegionToQImage: region.m_height={}, region.m_width={}, region.m_channels={}",
                 region.m_height, region.m_width, region.m_channels);
    spdlog::info("convertImageRegionToQImage: region.m_data.size()={}",
                 region.m_data.size());
    spdlog::info("convertImageRegionToQImage: Expected size={}",
                 region.m_width * region.m_height * region.m_channels);
    
    QImage::Format format;
    if (region.m_channels == 3) {
        format = QImage::Format_RGB888;
    } else if (region.m_channels == 4) {
        format = QImage::Format_RGBA8888;
    } else {
        spdlog::error("PaintedImageItem::convertImageRegionToQImage: Unsupported channel count: {}", 
                      region.m_channels);
        return QImage();
    }
    
    QImage qimg(region.m_width, region.m_height, format);
    
    if (region.m_channels == 3)
    {
        // RGB
        for (int y = 0; y < region.m_height; ++y)
        {

            uchar* scanLine = qimg.scanLine(y);
            
            for (int x = 0; x < region.m_width; ++x)
            {
                size_t srcIdx = (y * region.m_width + x) * 3;
                size_t dstIdx = x * 3;
                
                // Clamp and conversion float32 → uint8
                scanLine[dstIdx + 0] = static_cast<uchar>(std::clamp(region.m_data[srcIdx + 0], 0.0f, 1.0f) * 255.0f); // R
                scanLine[dstIdx + 1] = static_cast<uchar>(std::clamp(region.m_data[srcIdx + 1], 0.0f, 1.0f) * 255.0f); // G
                scanLine[dstIdx + 2] = static_cast<uchar>(std::clamp(region.m_data[srcIdx + 2], 0.0f, 1.0f) * 255.0f); // B
            }
        }
    } else if (region.m_channels == 4) {
        // RGBA
        for (int y = 0; y < region.m_height; ++y)
        {
            uchar* scanLine = qimg.scanLine(y);
            
            for (int x = 0; x < region.m_width; ++x)
            {
                size_t srcIdx = (y * region.m_width + x) * 4;
                size_t dstIdx = x * 4;
                
                scanLine[dstIdx + 0] = static_cast<uchar>(std::clamp(region.m_data[srcIdx + 0], 0.0f, 1.0f) * 255.0f); // R
                scanLine[dstIdx + 1] = static_cast<uchar>(std::clamp(region.m_data[srcIdx + 1], 0.0f, 1.0f) * 255.0f); // G
                scanLine[dstIdx + 2] = static_cast<uchar>(std::clamp(region.m_data[srcIdx + 2], 0.0f, 1.0f) * 255.0f); // B
                scanLine[dstIdx + 3] = static_cast<uchar>(std::clamp(region.m_data[srcIdx + 3], 0.0f, 1.0f) * 255.0f); // A
                if (y == 528 && x == 0) {  // Dernière ligne, premier pixel
                    spdlog::info("DEBUG y=528: srcIdx={}, dstIdx={}, scanLine ptr={:p}",
                                 srcIdx, dstIdx, (void*)scanLine);
                    spdlog::info("DEBUG y=528: Writing RGB = {:.0f},{:.0f},{:.0f}",
                                 region.m_data[srcIdx] * 255.0f,
                                 region.m_data[srcIdx+1] * 255.0f,
                                 region.m_data[srcIdx+2] * 255.0f);
                }
            }
        }

        spdlog::info("convertImageRegionToQImage: Last srcIdx processed={}",
                     ((region.m_height-1) * region.m_width + (region.m_width-1)) * 4);
    }
    
    spdlog::debug("PaintedImageItem::convertImageRegionToQImage: Converted {}x{} ({} channels) to QImage",
                  region.m_width, region.m_height, region.m_channels);
    return qimg;
}

bool PaintedImageItem::isImagePaintValid() const
{
    return (isImageValid() && !m_current_qimage.isNull());
}

// Sets the zoom level.
void PaintedImageItem::setZoom(float zoom)
{
    if (!qFuzzyCompare(m_zoom, zoom) && zoom > 0.0f)
    { // Verify the positive value zoom
        m_zoom = zoom;
        emit zoomChanged(m_zoom); // Emit signal for QML binding
        update(); // Trigger repaint
    }
}

// Sets the pan offset.
void PaintedImageItem::setPan(const QPointF& pan)
{
    if (m_pan != pan) {
        m_pan = pan;
        emit panChanged(m_pan); // Emit signal for QML binding
        update(); // Trigger repaint
    }
}

} // namespace CaptureMoment::UI::Rendering
