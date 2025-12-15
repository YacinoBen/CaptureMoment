/**
 * @file operation_provider.h
 * @brief Generic Qt base class for operations
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <QObject>
#include "image_controller.h"

namespace CaptureMoment::UI {
class ImageController;
/**
 * @brief Generic base class providing Qt signals/slots infrastructure
 * 
 * GENERIC - NO specific parameters (value, angle, radius, etc.)
 * Each derived class defines its OWN parameter setters/getters
 * 
 * This class provides ONLY the common Qt infrastructure:
 * - onOperationCompleted() / onOperationFailed() slots
 * - operationApplied() / operationFailed() signals
 * 
 * Parameter-specific methods (setValue, setAngle, setRadius, etc.)
 * are defined in each derived class
 */
class OperationProvider : public QObject {
    Q_OBJECT

public:
    explicit OperationProvider(QObject* parent = nullptr)
        : QObject(parent) {}

    virtual ~OperationProvider() = default;

protected : 
    /**
     * @brief Set ImageController reference
     */

    virtual void setImageController(ImageController* controller) = 0;
    /**
     * @brief Pointer to the ImageController responsible for coordinating operations.
     * This allows the operation model to communicate back to the central controller.
     */
    ImageController* m_image_controller {nullptr};

public slots:
    /**
     * @brief Handle operation completion
     * Generic - works for all operation types
     */
    virtual void onOperationCompleted() = 0;

    /**
     * @brief Handle operation failure
     * Generic - works for all operation types
     */
    virtual void onOperationFailed(const QString& error) = 0;

signals:

    /**
     * @brief Signal emitted when the operation's activity state changes.
     * This signal is emitted by the operation model itself (e.g., BrightnessModel)
     * when its internal state (like m_params) changes in a way that affects the result
     */
    void isActiveChanged(); 

    /**
     * @brief Operation applied successfully
     * Generic signal name
     */
    void operationApplied();

    /**
     * @brief Operation failed
     * Generic signal name
     */
    void operationFailed(QString error);
};

} // namespace CaptureMoment::UI
