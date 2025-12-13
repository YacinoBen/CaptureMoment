/**
 * @file ioperation_model.h
 * @brief Abstract base class defining the common interface for image operation models.
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <QObject>
#include <memory>
#include <string>
#include "operations/operation_descriptor.h"

namespace CaptureMoment::UI {

class ImageController;
    /**
     * @brief Abstract base class defining the common interface for image operation models.
     *
     * This class provides a standard interface for all image adjustment models (e.g., BrightnessModel,
     * ContrastModel, RotationModel). It defines common aspects like the operation's name,
     * its activity state, and its connection to the ImageController, while leaving the
     * specific parameter types and value handling to derived classes.
     *
     * Key benefits of this abstract base class:
     * - **Unified Interface:** QML components and controllers can treat all operation models uniformly
     *   for common operations (registration, getting name, checking activity).
     * - **Extensibility:** Adding new operations requires inheriting from this class and implementing
     *   the specific parameter handling.
     * - **Polymorphism:** The UI can hold lists of IOperationModel* and call common methods.
     * - **Separation of Concerns:** Defines the *common contract*, while derived classes implement
     *   the *specific parameter storage and behavior*.
     */
class IOperationModel : public QObject {
    Q_OBJECT

    // Expose only *common* properties here, or perhaps none at all in the base class.
    // Specific properties (like 'value', 'degrees', 'index') are defined in derived classes.
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(bool active READ isActive NOTIFY isActiveChanged)

public:
    /**
     * @brief Virtual destructor for safe inheritance.
     */
    virtual ~IOperationModel() = default;

    /**
     * @brief Pure virtual function to get the name of the operation.
     * @return QString representing the operation's name (e.g., "Brightness", "Rotation").
     */
    virtual QString name() const = 0;

    /**
     * @brief Pure virtual function to check if the operation is currently active.
     * An operation is considered active if its parameters differ from the default/no-op state.
     * @return true if the operation is active, false otherwise.
     */
    virtual bool isActive() const = 0;

    /**
     * @brief Pure virtual function to get the type of the operation.
     * @return OperationType representing the operation's category.
     */
    virtual OperationType getType() const = 0;

    /**
     * @brief Pure virtual function to get the current descriptor for this operation.
     * The descriptor contains the type, name, parameters, etc., ready to be sent to the core engine.
     * @return OperationDescriptor representing the current state of the operation.
     */
    virtual OperationDescriptor getDescriptor() const = 0;

    /**
     * @brief Pure virtual function to set the ImageController reference.
     * @param controller Pointer to the ImageController instance.
     */
    virtual void setImageController(ImageController* controller) = 0;

public slots:
    /**
     * @brief Resets the operation to its default state.
     * This slot resets the parameters to their neutral/default values.
     * It should emit relevant signals (e.g., valueChanged, isActiveChanged).
     */
    virtual void reset() = 0;

signals:
    /**
     * @brief Signal emitted when the operation's activity state changes.
     * QML can listen to this to update UI elements reflecting if the operation is active.
     */
    void isActiveChanged();

    /**
     * @brief Signal emitted when the operation has been successfully applied to the image.
     * QML can listen to this to provide feedback or trigger UI updates after processing.
     * This signal is typically emitted by the ImageController *after* the processing is complete.
     */
    void operationApplied();

    /**
     * @brief Signal emitted when the application of the operation fails.
     * QML can listen to this to display error messages.
     * This signal is typically emitted by the ImageController *after* the processing fails.
     * @param error A string describing the error that occurred.
     */
    void operationFailed(QString error);

protected:
    /**
     * @brief Pointer to the ImageController responsible for coordinating operations.
     * This allows the operation model to communicate back to the central controller.
     */
    ImageController* m_image_controller{nullptr};
    };

} // namespace CaptureMoment::UI
