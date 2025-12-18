/**
 * @file image_controller_painted.h
 * @brief 
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include "image_controller_base.h"
#include "rendering/painted_image_item.h"

namespace CaptureMoment::UI {

namespace Controller {

namespace Rendering {
  class PaintedmageItem;
}
/**
 * @class ImageControllerPainted
 * @brief Orchestrates Core processing and Qt UI updates with Paintedmethod
 * 
 */
class ImageControllerPainted : public ImageControllerBase {
    Q_OBJECT

    /**
     * @property paintedImageItem
     * @brief QML property exposing the PaintedImageItem pointer.
     *
     * This property allows QML components to get and set the PaintedImageItem
     * instance used for rendering. It facilitates the connection between
     * the UI layer (QML) and the rendering backend.
     *
     * Usage in QML:
     * @code
     * ImageControllerPainted {
     *     id: controller
     *     m_painted_image_item: paintedImageDisplay // Link to the PaintedImageItem instance in QML
     * }
     * @endcode
     */
    Q_PROPERTY(Rendering::PaintedImageItem* paintedImageItem READ getPaintedImageItem WRITE setPaintedImageItem NOTIFY paintedImageItemChanged)

public:
    /**
     * @brief Constructs ImageController
     * @param parent Parent QObject
     */
    explicit ImageControllerPainted(QObject* parent = nullptr);
    
    /**
     * @brief Destructor - cleans up worker thread
     */
    ~ImageControllerPainted();
    
    /**
     * @brief Set the Painted display item for rendering
     * @param item Pointer to PaintedImageItem
     */
    void setPaintedImageItem(Rendering::PaintedImageItem* item);
    
    /**
     * @brief Getter for the Painted image item property exposed to QML.
     *
     * This function is used by the Q_PROPERTY macro to provide read access
     * to the PaintedImageItem pointer from QML.
     *
     * @return A pointer to the currently associated PaintedImageItem, or nullptr if none is set.
     */
    Rendering::PaintedImageItem* getPaintedImageItem() const { return m_painted_image_item; }

    /**
     * @brief QML-accessible method to set the Painted image item.
     *
     * This method can be called directly from QML code using the
     * Component.onCompleted signal or other mechanisms to establish the
     * connection with the Painted image display item.
     *
     * @param item Pointer to the PaintedImageItem instance to be associated.
     */
    Q_INVOKABLE void setPaintedImageItemFromQml(Rendering::PaintedImageItem* item);

private:
    /**
     * @brief Painted display item (non-owned, set externally)
     */
    Rendering::PaintedImageItem* m_painted_image_item {nullptr};

signals:
    /**
     * @brief Signal emitted when the Painted image item property changes.
     *
     * This signal is connected to the Q_PROPERTY 'paintedImageItem' and is emitted
     * whenever the internal m_painted_image_item pointer is updated via the setter.
     * QML can use this signal to react to the change in the associated Painted item.
    */
    void paintedImageItemChanged();

protected :
    /**
     * @brief Perform actual image load (runs on worker thread).
     * Implementation specific to updating via PaintedImageItem.
     * @param filePath Path to the image file to load.
     */
    void doLoadImage(const QString& filePath) override;

    /**
     * @brief Perform actual operations (runs on worker thread).
     * Implementation specific to updating via PaintedImageItem.
     * @param operations Vector of operation descriptors to apply.
     */
    void doApplyOperations(const std::vector<OperationDescriptor>& operations) override;
};

} // namespace Controller

} // namespace CaptureMoment::Qt
