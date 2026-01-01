/**
 * @file image_controller_rhi.h
 * @brief Head for ImageControllerRHI from main class ImageControllerBase
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include "controller/image_controller_base.h"
#include "rendering/rhi_image_item.h"

namespace CaptureMoment::UI {
namespace Rendering {
  class RHIImageItem;
}
namespace Controller {

/**
 * @class ImageControllerRHI
 * @brief Orchestrates Core processing and Qt UI updates with RHImethod
 * 
 */
class ImageControllerRHI : public ImageControllerBase {
    Q_OBJECT

    /**
     * @property rhiImageItem
     * @brief QML property exposing the RHIImageItem pointer.
     *
     * This property allows QML components to get and set the RHIImageItem
     * instance used for rendering. It facilitates the connection between
     * the UI layer (QML) and the rendering backend.
     *
     * Usage in QML:
     * @code
     * ImageControllerRHI {
     *     id: controller
     *     m_rhi_image_item: rhiImageDisplay // Link to the RHIImageItem instance in QML
     * }
     * @endcode
     */
    Q_PROPERTY(CaptureMoment::UI::Rendering::RHIImageItem* rhiImageItem READ getRHIImageItem WRITE setRHIImageItem NOTIFY rhiImageItemChanged)

public:
    /**
     * @brief Constructs ImageController
     * @param parent Parent QObject
     */
    explicit ImageControllerRHI(QObject* parent = nullptr);
    
    /**
     * @brief Destructor - cleans up worker thread
     */
    ~ImageControllerRHI();
    
    /**
     * @brief Set the RHI display item for rendering
     * @param item Pointer to RHIImageItem
     */
    void setRHIImageItem(CaptureMoment::UI::Rendering::RHIImageItem* item);
    
    /**
     * @brief Getter for the RHI image item property exposed to QML.
     *
     * This function is used by the Q_PROPERTY macro to provide read access
     * to the RHIImageItem pointer from QML.
     *
     * @return A pointer to the currently associated RHIImageItem, or nullptr if none is set.
     */
    CaptureMoment::UI::Rendering::RHIImageItem* getRHIImageItem() const { return m_rhi_image_item; }

    /**
     * @brief QML-accessible method to set the RHI image item.
     *
     * This method can be called directly from QML code using the
     * Component.onCompleted signal or other mechanisms to establish the
     * connection with the RHI image display item.
     *
     * @param item Pointer to the RHIImageItem instance to be associated.
     */
    Q_INVOKABLE void setRHIImageItemFromQml(CaptureMoment::UI::Rendering::RHIImageItem* item);

private:
    /**
     * @brief RHI display item (non-owned, set externally)
     */
    CaptureMoment::UI::Rendering::RHIImageItem* m_rhi_image_item {nullptr};

signals:
    /**
     * @brief Signal emitted when the RHI image item property changes.
     *
     * This signal is connected to the Q_PROPERTY 'RHIImageItem' and is emitted
     * whenever the internal m_rhi_image_item pointer is updated via the setter.
     * QML can use this signal to react to the change in the associated RHId item.
    */
    void rhiImageItemChanged();
};

} // namespace Controller

} // namespace CaptureMoment::Qt
