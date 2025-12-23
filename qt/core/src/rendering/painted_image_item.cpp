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

    // Constructor: Initializes the item and sets the flag for custom content.
    PaintedImageItem::PaintedImageItem(QQuickItem* parent)
        : QQuickPaintedItem(parent) // Appel le constructeur de la base QQuickPaintedItem
    {
        // Indicate to Qt Quick that this item has custom content rendered via QPainter.
        // setFlag(QQuickItem::ItemHasContents, true); // Implicitement vrai pour QQuickPaintedItem
        setSize(QSizeF(800, 600)); // Taille par défaut
        spdlog::info("PaintedImageItem: Created");
    }

    // Sets the full image to be displayed.
    // Updates the internal image data and schedules a repaint.
    void PaintedImageItem::setImage(const std::shared_ptr<ImageRegion>& image) 
    {
        if (!image || !image->isValid())
        {
            spdlog::warn("PaintedImageItem::setImage: Invalid image region");
            return;
        }
        
        spdlog::info("PaintedImageItem::setImage: {}x{}", image->m_width, image->m_height);

        // Protect access to shared data (m_current_qimage, m_image_width, m_image_height)
        {
            QMutexLocker lock(&m_image_mutex);
            // Convert ImageRegion to QImage
            m_current_qimage = convertImageRegionToQImage(*image); // Passer par la référence
            m_image_width = image->m_width;
            m_image_height = image->m_height;
        }

        // Emit signal for QML binding
        emit imageDimensionsChanged();

        // Trigger a repaint to reflect the new image.
        update();
    }

    // Updates a specific tile of the displayed image.
    // Merges the tile data into the internal QImage buffer and schedules a repaint.
    void PaintedImageItem::updateTile(const std::shared_ptr<ImageRegion>& tile)
    {
         if (!tile || !tile->isValid()) {
        spdlog::warn("PaintedImageItem::updateTile: Invalid tile");
        return;
    }

    QMutexLocker lock(&m_image_mutex);

    if (m_current_qimage.isNull()) {
        spdlog::warn("PaintedImageItem::updateTile: No base image loaded");
        return;
    }

    // Check bounds
    if (tile->m_x < 0 || tile->m_y < 0 ||
        tile->m_x + tile->m_width > m_current_qimage.width() ||
        tile->m_y + tile->m_height > m_current_qimage.height()) {
        spdlog::warn("PaintedImageItem::updateTile: Tile out of bounds");
        return;
    }

    const int expectedChannels = (m_current_qimage.format() == QImage::Format_RGB888) ? 3 : 4;
    if (tile->m_channels != expectedChannels) {
        spdlog::error("PaintedImageItem::updateTile: Channel mismatch. Expected {}, got {}", 
                     expectedChannels, tile->m_channels);
        return;
    }

    uchar* scanLine {nullptr};

    if (tile->m_channels == 3) {
        // RGB
        for (int y = 0; y < tile->m_height; ++y) {
            int imgY = tile->m_y + y;
            scanLine = m_current_qimage.scanLine(imgY);
            
            for (int x = 0; x < tile->m_width; ++x) {
                int imgX = tile->m_x + x;
                size_t srcIdx = (y * tile->m_width + x) * 3;
                size_t dstIdx = imgX * 3;

                if (srcIdx + 2 < tile->m_data.size()) {
                    scanLine[dstIdx + 0] = static_cast<uchar>(std::clamp(tile->m_data[srcIdx + 0], 0.0f, 1.0f) * 255.0f);
                    scanLine[dstIdx + 1] = static_cast<uchar>(std::clamp(tile->m_data[srcIdx + 1], 0.0f, 1.0f) * 255.0f);
                    scanLine[dstIdx + 2] = static_cast<uchar>(std::clamp(tile->m_data[srcIdx + 2], 0.0f, 1.0f) * 255.0f);
                }
            }
        }
    } else if (tile->m_channels == 4) {
        // RGBA
        for (int y = 0; y < tile->m_height; ++y) {
            int imgY = tile->m_y + y;
            scanLine = m_current_qimage.scanLine(imgY);
            
            for (int x = 0; x < tile->m_width; ++x) {
                int imgX = tile->m_x + x;
                size_t srcIdx = (y * tile->m_width + x) * 4;
                size_t dstIdx = imgX * 4;
                
                if (srcIdx + 3 < tile->m_data.size()) {
                    scanLine[dstIdx + 0] = static_cast<uchar>(std::clamp(tile->m_data[srcIdx + 0], 0.0f, 1.0f) * 255.0f);
                    scanLine[dstIdx + 1] = static_cast<uchar>(std::clamp(tile->m_data[srcIdx + 1], 0.0f, 1.0f) * 255.0f);
                    scanLine[dstIdx + 2] = static_cast<uchar>(std::clamp(tile->m_data[srcIdx + 2], 0.0f, 1.0f) * 255.0f);
                    scanLine[dstIdx + 3] = static_cast<uchar>(std::clamp(tile->m_data[srcIdx + 3], 0.0f, 1.0f) * 255.0f);
                }
            }
        }
    }
    
    spdlog::info("PaintedImageItem::updateTile: Updated tile at ({}, {}) {}x{}",
                 tile->m_x, tile->m_y, tile->m_width, tile->m_height);

    spdlog::info("PaintedImageItem::updateTile: Sample pixel RGB: {},{},{}",
                 scanLine[0], scanLine[1], scanLine[2]);

    update();
    }

    // Sets the zoom level.
    void PaintedImageItem::setZoom(float zoom) 
    {
        if (!qFuzzyCompare(m_zoom, zoom) && zoom > 0.0f) { // Vérifiez que le zoom est positif
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

    // Gets the width of the image.
    int PaintedImageItem::imageWidth() const {
        QMutexLocker lock(&m_image_mutex);
        return m_image_width;
    }

    // Gets the height of the image.
    int PaintedImageItem::imageHeight() const {
        QMutexLocker lock(&m_image_mutex);
        return m_image_height;
    }

    // Paints the image using QPainter.
    void PaintedImageItem::paint(QPainter* painter) {
        if (!painter) {
            spdlog::error("PaintedImageItem::paint: QPainter is null");
            return;
        }

        // Protect access to the shared image data during painting
        QMutexLocker lock(&m_image_mutex);

        if (m_current_qimage.isNull()) {
            spdlog::warn("PaintedImageItem::paint: No image to paint");
            return;
        }

        // Save the current painter state
        painter->save();

        // Apply zoom and pan transformations
        painter->translate(m_pan); // Appliquer le décalage (pan)
        painter->scale(m_zoom, m_zoom); // Appliquer le zoom (uniforme pour l'instant)

        // Draw the image
        // Dessine l'image à la position (0, 0) dans le système de coordonnées transformé
        painter->drawImage(QPointF(0, 0), m_current_qimage);

        // Restore the painter state
        painter->restore();

        spdlog::trace("PaintedImageItem::paint: Frame painted");
    }

    // Converts an ImageRegion to a QImage.
    QImage PaintedImageItem::convertImageRegionToQImage(const ImageRegion& region) const {
       if (!region.isValid()) {
        spdlog::error("PaintedImageItem::convertImageRegionToQImage: Invalid region");
        return QImage();
    }
    
    // ✅ Choisir le format QImage selon le nombre de canaux
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
    
    // ✅ Conversion adaptée au nombre de canaux
    if (region.m_channels == 3) {
        // RGB uniquement
        for (int y = 0; y < region.m_height; ++y) {
            // Accès direct à la ligne pour performance
            uchar* scanLine = qimg.scanLine(y);
            
            for (int x = 0; x < region.m_width; ++x) {
                size_t srcIdx = (y * region.m_width + x) * 3; // ✅ 3 canaux
                size_t dstIdx = x * 3;
                
                // Clamp et conversion float32 → uint8
                scanLine[dstIdx + 0] = static_cast<uchar>(std::clamp(region.m_data[srcIdx + 0], 0.0f, 1.0f) * 255.0f); // R
                scanLine[dstIdx + 1] = static_cast<uchar>(std::clamp(region.m_data[srcIdx + 1], 0.0f, 1.0f) * 255.0f); // G
                scanLine[dstIdx + 2] = static_cast<uchar>(std::clamp(region.m_data[srcIdx + 2], 0.0f, 1.0f) * 255.0f); // B
            }
        }
    } else if (region.m_channels == 4) {
        // RGBA
        for (int y = 0; y < region.m_height; ++y) {
            uchar* scanLine = qimg.scanLine(y);
            
            for (int x = 0; x < region.m_width; ++x) {
                size_t srcIdx = (y * region.m_width + x) * 4; // ✅ 4 canaux
                size_t dstIdx = x * 4;
                
                scanLine[dstIdx + 0] = static_cast<uchar>(std::clamp(region.m_data[srcIdx + 0], 0.0f, 1.0f) * 255.0f); // R
                scanLine[dstIdx + 1] = static_cast<uchar>(std::clamp(region.m_data[srcIdx + 1], 0.0f, 1.0f) * 255.0f); // G
                scanLine[dstIdx + 2] = static_cast<uchar>(std::clamp(region.m_data[srcIdx + 2], 0.0f, 1.0f) * 255.0f); // B
                scanLine[dstIdx + 3] = static_cast<uchar>(std::clamp(region.m_data[srcIdx + 3], 0.0f, 1.0f) * 255.0f); // A
            }
        }
    }
    
    spdlog::debug("PaintedImageItem::convertImageRegionToQImage: Converted {}x{} ({} channels) to QImage",
                 region.m_width, region.m_height, region.m_channels);
    return qimg;
    }

} // namespace CaptureMoment::UI::Rendering
