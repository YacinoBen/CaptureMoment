/**
 * @file image_controller.h
 * @brief Qt bridge between Core (PhotoEngine) and Qt UI
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <QObject>
#include <memory>
#include <QThread>
#include "engine/photo_engine.h"
#include "common/image_region.h"
#include "rendering/rhi_image_item.h"
#include "models/operations/i_operation_model.h"

namespace CaptureMoment::UI {
class IOperationModel;

namespace Rendering {
  class RHIImageItem;
}
/**
 * @class ImageController
 * @brief Orchestrates Core processing and Qt UI updates
 * 
 * Responsibilities:
 * - Owns PhotoEngine (Core)
 * - Manages worker thread for non-blocking operations
 * - Exposes Qt slots for QML/UI
 * - Emits Qt signals for UI updates
 * - Thread-safe image and operation handling
 */
class ImageController : public QObject {
    Q_OBJECT

private:
    /**
     * @brief Core processing engine
     */
    std::shared_ptr<PhotoEngine> m_engine;
    
    /**
     * @brief Worker thread for non-blocking operations
     */
    QThread m_worker_thread;
    
    /**
     * @brief Current loaded image
     */
    std::shared_ptr<ImageRegion> m_current_image;

    /**
     * @brief RHI display item (non-owned, set externally)
     */
    Rendering::RHIImageItem* m_rhi_image_item{nullptr};
    
    /**
     * @brief Current image dimensions
     */
    int m_image_width{0};
    int m_image_height{0};

    /**
     * @brief Registered operation models for notifications
     * These models will receive operationCompleted/operationFailed signals
     */  
    std::vector<IOperationModel*> m_registered_models;
public:
    /**
     * @brief Constructs ImageController
     * @param parent Parent QObject
     */
    explicit ImageController(QObject* parent = nullptr);
    
    /**
     * @brief Destructor - cleans up worker thread
     */
    ~ImageController();
    
    /**
     * @brief Set the RHI display item for rendering
     * @param item Pointer to RHIImageItem
     */
    void setRHIImageItem(Rendering::RHIImageItem* item);
    
    /**
     * @brief Get current image width
     */
    int imageWidth() const noexcept { return m_image_width; }
    
    /**
     * @brief Get current image height
     */
    int imageHeight() const noexcept { return m_image_height; }
    
    /**
     * @brief Get current loaded image
     */
    std::shared_ptr<ImageRegion> currentImage() const { return m_current_image; }


    /**
     * @brief Register an operation model for notifications
     * 
     * Called by IOperationModel::setImageController()
     * The model will receive operationCompleted/operationFailed signals
     * after operations are processed.
     * 
     * @param model Pointer to IOperationModel
     */
    void registerModel(IOperationModel* model);

    /**
     * @brief Getter for the RHI image item property exposed to QML.
     *
     * This function is used by the Q_PROPERTY macro to provide read access
     * to the RHIImageItem pointer from QML.
     *
     * @return A pointer to the currently associated RHIImageItem, or nullptr if none is set.
     */
    Rendering::RHIImageItem* getRHIImageItem() const { return m_rhi_image_item; }

    /**
     * @property rhiImageItem
     * @brief QML property exposing the RHIImageItem pointer.
     *
     * This property allows QML components to get and set the RHIImageItem
     * instance used for rendering. It facilitates the connection between
     * the UI layer (QML) and the rendering backend (QRhi via RHIImageItem/RHIImageNode).
     *
     * Usage in QML:
     * @code
     * ImageController {
     *     id: controller
     *     rhiImageItem: rhiImageDisplay // Link to the RHIImageItem instance in QML
     * }
     * @endcode
     */
    Q_PROPERTY(Rendering::RHIImageItem* rhiImageItem READ getRHIImageItem WRITE setRHIImageItem NOTIFY rhiImageItemChanged)

    /**
     * @brief QML-accessible method to set the RHI image item.
     *
     * This method can be called directly from QML code using the
     * Component.onCompleted signal or other mechanisms to establish the
     * connection with the RHI image display item.
     *
     * @param item Pointer to the RHIImageItem instance to be associated.
     */
    Q_INVOKABLE void setRHIImageItemFromQml(Rendering::RHIImageItem* item);

public slots:
    /**
     * @brief Load image from file path (non-blocking)
     * @param filePath Path to image file
     */
    void loadImage(const QString& filePath);
    
    /**
     * @brief Apply operation with parameters (non-blocking)
     * @param operations Vector of opstd::shared_ptr<ImageRegion>eration descriptors
     */
    void applyOperations(const std::vector<OperationDescriptor>& operations);
    /**
     * @brief Internal: Handle load image result
     * @param success Whether load succeeded
     * @param errorMsg Error message if failed
     */
    void onImageLoadResult(bool success, const QString& errorMsg);

    /**
     * @brief Internal: Handle operation result
     * @param success Whether operation succeeded
     * @param errorMsg Error message if failed
     */
    void onOperationResult(bool success, const QString& errorMsg);

signals:
    /**
     * @brief Emitted when image loaded successfully
     * @param width Image width
     * @param height Image height
     */
    void imageLoaded(int width, int height);
    
    /**
     * @brief Emitted when image load fails
     * @param error Error message
     */
    void imageLoadFailed(QString error);
    
    /**
     * @brief Emitted when operation completes
     */
    void operationCompleted();
    
    /**
     * @brief Emitted when operation fails
     * @param error Error message
     */
    void operationFailed(QString error);

    /**
     * @brief Signal emitted when the RHI image item property changes.
     *
     * This signal is connected to the Q_PROPERTY 'rhiImageItem' and is emitted
     * whenever the internal m_rhi_image_item pointer is updated via the setter.
     * QML can use this signal to react to the change in the associated RHI item.
    */
    void rhiImageItemChanged();
    
private:
    /**
     * @brief Perform actual image load (runs on worker thread)
     */
    void doLoadImage(const QString& filePath);
    
    /**
     * @brief Perform actual operations (runs on worker thread)
     */
    void doApplyOperations(const std::vector<OperationDescriptor>& operations);
};

} // namespace CaptureMoment::Qt
