/**
 * @file operation_provider.h
 * @brief Generic Qt base class for operation models with QObject infrastructure.
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <QObject>
#include <spdlog/spdlog.h>
#include "controller/image_controller_base.h"
#include "models/operations/i_operation_model.h"

namespace CaptureMoment::UI {

namespace Controller {
class ImageControllerBase;
}

namespace Models::Operations {

/**
 * @brief Generic base class providing Qt signals/slots infrastructure for operation models.
 *
 * This class serves as an intermediate layer between the pure interface IOperationModel
 * and concrete operation models (like BrightnessModel, ContrastModel, etc.).
 * It inherits from QObject to provide Qt's signal/slot mechanism and from IOperationModel
 * to fulfill the operation contract.
 *
 * OperationProvider handles common Qt-related tasks such as:
 * - Defining generic signals (`operationApplied`, `operationFailed`, `isActiveChanged`).
 * - Declaring virtual slots for handling operation results (`onOperationCompleted`, `onOperationFailed`).
 * - Providing a protected member (`m_image_controller`) to store a reference to the controller.
 * - Declaring a pure virtual method (`setImageController`) for controller setup.
 *
 * Concrete operation models should inherit from OperationProvider to gain QObject
 * functionality and implement the IOperationModel interface.
 */
class OperationProvider : public QObject, public IOperationModel {
    Q_OBJECT

public:
    /**
     * @brief Constructs an OperationProvider.
     * @param parent Optional parent QObject.
     */
    explicit OperationProvider(QObject* parent = nullptr){ spdlog::debug("OperationProvider: Constructed"); };

    /**
     * @brief Virtual destructor.
     */
    virtual ~OperationProvider() = default;

protected:
    /**
     * @brief Pure virtual method to set the ImageControllerBase reference.
     *
     * Derived classes must implement this method to establish communication
     * with the central controller. It typically involves storing the controller pointer
     * and connecting signals/slots.
     *
     * @param controller Pointer to the ImageControllerBase instance.
     */
    virtual void setImageController(Controller::ImageControllerBase* controller) = 0;

    /**
     * @brief Pointer to the ImageController responsible for coordinating operations.
     * This allows the operation model to communicate back to the central controller.
     * It should be set by the implementation of setImageController in derived classes.
     */
    Controller::ImageControllerBase* m_image_controller {nullptr};

public slots:
    /**
     * @brief Pure virtual slot to handle successful operation completion.
     * Implementations in derived classes should react to a successful operation
     * (e.g., emit UI-specific signals, log success).
     */
    virtual void onOperationCompleted() = 0;

    /**
     * @brief Pure virtual slot to handle operation failure.
     * Implementations in derived classes should react to a failed operation
     * (e.g., emit UI-specific signals with error details, log error).
     *
     * @param error A string describing the error that occurred.
     */
    virtual void onOperationFailed(const QString& error) = 0;

signals:
    /**
     * @brief Signal emitted when the operation's activity state changes.
     * This signal is emitted by the operation model itself (e.g., a derived class like BrightnessModel)
     * when its internal state (like m_params) changes in a way that affects whether the operation
     * should be considered active.
     */
    void isActiveChanged();

    /**
     * @brief Signal emitted when an operation is applied successfully.
     * Generic signal name, can be connected by UI components or other listeners.
     */
    void operationApplied();

    /**
     * @brief Signal emitted when an operation fails.
     * Generic signal name, can be connected by UI components or other listeners.
     *
     * @param error A string describing the error that occurred.
     */
    void operationFailed(QString error);
};

} // namespace Models::Operations

} // namespace CaptureMoment::UI
