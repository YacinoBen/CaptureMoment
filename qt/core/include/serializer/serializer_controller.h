/**
 * @file serializer_controller.h
 * @brief UI manager for handling file-based serialization and deserialization of image operations.
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include "serializer/file_serializer_manager.h"
#include "operations/operation_descriptor.h"

#include <QObject>
#include <QString>
#include <memory>
#include <vector>

namespace CaptureMoment::Core {
    namespace Operations {
        struct OperationDescriptor;
    }
    namespace Serializer {
        class FileSerializerManager;
    }
}

namespace CaptureMoment::UI {

namespace Serializer {

/**
 * @brief UI manager for handling file-based serialization and deserialization of image operations.
 *
 * This class acts as a Qt/QML-friendly wrapper around the core CaptureMoment::Core::Serializer::FileSerializerManager.
 * It provides slots for saving and loading operations to/from XMP files associated with image paths.
 * It emits Qt signals to notify the UI layer about the success or failure of these operations.
 * This class is designed to be exposed to QML and operates within the UI thread.
 * It is fully independent of PhotoEngine and only deals with serialization logic.
 */
class SerializerController : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a UISerializerManager.
     *
     * Initializes the manager with the core FileSerializerManager dependency.
     * This dependency is typically created and injected by a higher-level application module
     * responsible for assembling the components.
     *
     * @param file_serializer_manager A unique pointer to the core FileSerializerManager instance.
     *        Must not be null.
     * @param parent Optional parent QObject.
     * @throws std::invalid_argument if file_serializer_manager is null.
     */
    explicit SerializerController(std::unique_ptr<CaptureMoment::Core::Serializer::FileSerializerManager> file_serializer_manager, QObject* parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~SerializerController() override;

    // Disable copy and move semantics for simplicity if the core dependency is unique_ptr
    SerializerController(const SerializerController&) = delete;
    SerializerController& operator=(const SerializerController&) = delete;
    SerializerController(SerializerController&&) = delete;
    SerializerController& operator=(SerializerController&&) = delete;

signals:
    /**
     * @brief Signal emitted when operations are successfully saved to a file.
     */
    void operationsSaved();

    /**
     * @brief Signal emitted when the operation to save operations fails.
     * @param error_msg A human-readable description of the error.
     */
    void operationsSaveFailed(const QString& error_msg);

    /**
     * @brief Signal emitted when operations are successfully loaded from a file.
     * @param operations The vector of OperationDescriptors loaded from the file.
     */
    void operationsLoaded(const std::vector<CaptureMoment::Core::Operations::OperationDescriptor>& operations);

    /**
     * @brief Signal emitted when no operations were found in the file during load.
     */
    void operationsNotFoundOnLoad();

    /**
     * @brief Signal emitted when the operation to load operations fails.
     * @param error_msg A human-readable description of the error.
     */
    void operationsLoadFailed(const QString& error_msg);

public slots:
    /**
     * @brief Saves the provided list of operations to a file associated with the given image path.
     *
     * This slot calls the core FileSerializerManager to perform the save operation.
     * It emits either operationsSaved() or operationsSaveFailed() upon completion.
     *
     * @param image_path The path to the source image file. Used to determine the target XMP file location.
     * @param operations The vector of OperationDescriptors to serialize.
     */
    void saveOperations(const QString& image_path, const std::vector<CaptureMoment::Core::Operations::OperationDescriptor>& operations);

    /**
     * @brief Loads a list of operations from a file associated with the given image path.
     *
     * This slot calls the core FileSerializerManager to perform the load operation.
     * It emits either operationsLoaded(operations) or operationsNotFoundOnLoad() or operationsLoadFailed() upon completion.
     * The loaded operations are transmitted via the operationsLoaded signal.
     *
     * @param image_path The path to the source image file. Used to determine the source XMP file location.
     */
    void loadOperations(const QString& image_path);

private:
    /**
     * @brief Unique pointer to the core FileSerializerManager instance.
     * This manager handles the actual serialization logic using Exiv2, path strategies, etc.
     */
    std::unique_ptr<CaptureMoment::Core::Serializer::FileSerializerManager> m_file_serializer_manager;
};

} // namespace Serializer

} // namespace CaptureMoment::UI
