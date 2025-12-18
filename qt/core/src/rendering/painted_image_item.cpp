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
#include <algorithm> // Pour std::clamp
#include <cstring>   // Pour memcpy si nécessaire

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
        if (!tile || !tile->isValid())
        {
            spdlog::warn("PaintedImageItem::updateTile: Invalid tile");
            return;
        }

        // Protect access to shared data (m_current_qimage)
        {
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

            // Iterate through the pixels of the tile and update the QImage.
            // This assumes the internal QImage format matches the conversion logic in convertImageRegionToQImage.
            // QImage::scanLine(y) peut être plus efficace que setPixel pour de grands blocs.
            for (int y = 0; y < tile->m_height; ++y) {
                for (int x = 0; x < tile->m_width; ++x) {
                    // Convertir la valeur du pixel du tile (ImageRegion) en QRgb pour QImage
                    QRgb pixelValue = 0;
                    size_t baseIdx = (y * tile->m_width + x) * 4; // Supposons 4 canaux (RGBA)
                    if (baseIdx + 3 < tile->m_data.size()) {
                        float r = std::clamp(tile->m_data[baseIdx + 0], 0.0f, 1.0f);
                        float g = std::clamp(tile->m_data[baseIdx + 1], 0.0f, 1.0f);
                        float b = std::clamp(tile->m_data[baseIdx + 2], 0.0f, 1.0f);
                        float a = std::clamp(tile->m_data[baseIdx + 3], 0.0f, 1.0f); // Supposons un canal alpha

                        pixelValue = qRgba(
                            static_cast<uchar>(r * 255),
                            static_cast<uchar>(g * 255),
                            static_cast<uchar>(b * 255),
                            static_cast<uchar>(a * 255)
                        );
                    }
                    // Mettre à jour le pixel dans l'image interne
                    m_current_qimage.setPixelColor(tile->m_x + x, tile->m_y + y, QColor(pixelValue));
                }
            }
        }
        
        spdlog::debug("PaintedImageItem::updateTile: Merged tile at ({}, {}) {}x{}", 
                     tile->m_x, tile->m_y, tile->m_width, tile->m_height);
        // Trigger a repaint to reflect the updated tile.
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
        // QImage::Format_RGBA8888_Premultiplied est souvent un bon choix pour des données RGBA F32 converties en uint8
        // Si votre ImageRegion est RGB sans alpha, utilisez Format_RGB888 et adaptez la conversion.
        QImage qimg(region.m_width, region.m_height, QImage::Format_RGBA8888); // Ou Format_ARGB32_Premultiplied

        for (int y = 0; y < region.m_height; ++y) {
            for (int x = 0; x < region.m_width; ++x) {
                QRgb pixelValue = 0;
                size_t baseIdx = (y * region.m_width + x) * 4; // Supposons 4 canaux (RGBA)
                if (baseIdx + 3 < region.m_data.size()) {
                    float r = std::clamp(region.m_data[baseIdx + 0], 0.0f, 1.0f);
                    float g = std::clamp(region.m_data[baseIdx + 1], 0.0f, 1.0f);
                    float b = std::clamp(region.m_data[baseIdx + 2], 0.0f, 1.0f);
                    float a = std::clamp(region.m_data[baseIdx + 3], 0.0f, 1.0f); // Supposons un canal alpha

                    pixelValue = qRgba(
                        static_cast<uchar>(r * 255),
                        static_cast<uchar>(g * 255),
                        static_cast<uchar>(b * 255),
                        static_cast<uchar>(a * 255)
                    );
                }
                qimg.setPixelColor(x, y, QColor(pixelValue));
            }
        }

        spdlog::debug("PaintedImageItem::convertImageRegionToQImage: Converted {}x{} region to QImage",
                     region.m_width, region.m_height);
        return qimg;
    }

} // namespace CaptureMoment::UI::Rendering
