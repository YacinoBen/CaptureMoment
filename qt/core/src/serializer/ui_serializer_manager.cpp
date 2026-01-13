// ui/src/controller/serializer/ui_serializer_manager.cpp
#include "serializer/ui_serializer_manager.h"
#include <spdlog/spdlog.h>

namespace CaptureMoment::UI {

namespace Serializer {

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
UISerializerManager::UISerializerManager(std::unique_ptr<CaptureMoment::Core::Serializer::FileSerializerManager> file_serializer_manager, QObject* parent)
    : QObject(parent), m_file_serializer_manager(std::move(file_serializer_manager)) {
    if (!m_file_serializer_manager) {
        spdlog::error("UISerializerManager: Constructor received a null FileSerializerManager.");
        throw std::invalid_argument("UISerializerManager: FileSerializerManager cannot be null.");
    }
    spdlog::debug("UISerializerManager: Constructed with FileSerializerManager.");
}

/**
 * @brief Destructor.
 */
UISerializerManager::~UISerializerManager() {
    spdlog::debug("UISerializerManager: Destroyed.");
}

/**
 * @brief Saves the provided list of operations to a file associated with the given image path.
 *
 * This slot calls the core FileSerializerManager to perform the save operation.
 * It emits either operationsSaved() or operationsSaveFailed() upon completion.
 *
 * @param image_path The path to the source image file. Used to determine the target XMP file location.
 * @param operations The vector of OperationDescriptors to serialize.
 */
void UISerializerManager::saveOperations(const QString& image_path, const std::vector<CaptureMoment::Core::Operations::OperationDescriptor>& operations) {
    if (!m_file_serializer_manager)
    {
        spdlog::error("UISerializerManager::saveOperations: FileSerializerManager is null.");
        emit operationsSaveFailed("Internal error: Serialization manager not initialized.");
        return;
    }

    if (image_path.isEmpty())
    {
        spdlog::error("UISerializerManager::saveOperations: Image path is empty.");
        emit operationsSaveFailed("Image path is empty.");
        return;
    }

    spdlog::debug("UISerializerManager::saveOperations: Attempting to save {} operations for image: {}", operations.size(), image_path.toStdString());

    bool success = m_file_serializer_manager->saveToFile(image_path.toStdString(), operations);

    if (success) {
        spdlog::info("UISerializerManager::saveOperations: Successfully saved operations for image: {}", image_path.toStdString());
        emit operationsSaved();
    } else {
        spdlog::error("UISerializerManager::saveOperations: Failed to save operations for image: {}", image_path.toStdString());
        emit operationsSaveFailed("Failed to write operations to file.");
    }
}

/**
 * @brief Loads a list of operations from a file associated with the given image path.
 *
 * This slot calls the core FileSerializerManager to perform the load operation.
 * It emits either operationsLoaded(operations) or operationsNotFoundOnLoad() or operationsLoadFailed() upon completion.
 * The loaded operations are transmitted via the operationsLoaded signal.
 *
 * @param image_path The path to the source image file. Used to determine the source XMP file location.
 */
void UISerializerManager::loadOperations(const QString& image_path) {
    if (!m_file_serializer_manager)
    {
        spdlog::error("UISerializerManager::loadOperations: FileSerializerManager is null.");
        emit operationsLoadFailed("Internal error: Serialization manager not initialized.");
        return;
    }

    if (image_path.isEmpty())
    {
        spdlog::error("UISerializerManager::loadOperations: Image path is empty.");
        emit operationsLoadFailed("Image path is empty.");
        return;
    }

    spdlog::debug("UISerializerManager::loadOperations: Attempting to load operations for image: {}", image_path.toStdString());

    std::vector<CaptureMoment::Core::Operations::OperationDescriptor> loaded_ops = m_file_serializer_manager->loadFromFile(image_path.toStdString());

    if (loaded_ops.empty())
    {
        spdlog::info("UISerializerManager::loadOperations: No operations loaded from file for image: {} (file might not exist or be empty)", image_path.toStdString());
        emit operationsNotFoundOnLoad();
        return;
    }

    spdlog::info("UISerializerManager::loadOperations: Successfully loaded {} operations from file for image: {}", loaded_ops.size(), image_path.toStdString());
    emit operationsLoaded(loaded_ops);
}

} // namespace Serializer

} // namespace CaptureMoment::UI
