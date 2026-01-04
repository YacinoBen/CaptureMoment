/**
 * @file image_controller_sgs.h
 * @brief Head for ImageControllerSGS from main class ImageControllerBase
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include "controller/image_controller_base.h"
#include "rendering/sgs_image_item.h"

namespace CaptureMoment::UI {
    
namespace Rendering {
  class SGSImageItem;
}

namespace Controller {
/**
 * @class ImageControllerSGS
 * @brief Orchestrates Core processing and Qt UI updates with SGSmethod
 * 
 */
class ImageControllerSGS : public ImageControllerBase {
    Q_OBJECT

    /**
     * @property sgsImageItem
     * @brief QML property exposing the SGSImageItem pointer.
     *
     * This property allows QML components to get and set the SGSImageItem
     * instance used for rendering. It facilitates the connection between
     * the UI layer (QML) and the rendering backend.
     *
     * Usage in QML:
     * @code
     * ImageControllerSGS {
     *     id: controller
     *     m_sgs_image_item: sgsImageDisplay // Link to the SGSImageItem instance in QML
     * }
     * @endcode
     */
    Q_PROPERTY(CaptureMoment::UI::Rendering::SGSImageItem* sgsImageItem READ getSGSImageItem WRITE setSGSImageItem NOTIFY sgsImageItemChanged)

public:
    /**
     * @brief Constructs ImageController
     * @param parent Parent QObject
     */
    explicit ImageControllerSGS(QObject* parent = nullptr);
    
    /**
     * @brief Destructor - cleans up worker thread
     */
    ~ImageControllerSGS();
    
    /**
     * @brief Set the SGS display item for rendering
     * @param item Pointer to SGSImageItem
     */
    void setSGSImageItem(CaptureMoment::UI::Rendering::SGSImageItem* item);
    
    /**
     * @brief Getter for the SGS image item property exposed to QML.
     *
     * This function is used by the Q_PROPERTY macro to provide read access
     * to the SGSImageItem pointer from QML.
     *
     * @return A pointer to the currently associated SGSImageItem, or nullptr if none is set.
     */
    CaptureMoment::UI::Rendering::SGSImageItem* getSGSImageItem() const { return m_sgs_image_item; }

    /**
     * @brief QML-accessible method to set the SGS image item.
     *
     * This method can be called directly from QML code using the
     * Component.onCompleted signal or other mechanisms to establish the
     * connection with the SGS image display item.
     *
     * @param item Pointer to the SGSImageItem instance to be associated.
     */
    Q_INVOKABLE void setSGSImageItemFromQml(CaptureMoment::UI::Rendering::SGSImageItem* item);

private:
    /**
     * @brief SGS display item (non-owned, set externally)
     */
    CaptureMoment::UI::Rendering::SGSImageItem* m_sgs_image_item {nullptr};

signals:
    /**
     * @brief Signal emitted when the SGS image item property changes.
     *
     * This signal is connected to the Q_PROPERTY 'SGSImageItem' and is emitted
     * whenever the internal m_sgs_image_item pointer is updated via the setter.
     * QML can use this signal to react to the change in the associated SGSd item.
    */
    void sgsImageItemChanged();
};

} // namespace Controller

} // namespace CaptureMoment::Qt
