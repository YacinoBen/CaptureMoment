# 🏗️ Qt::Core & Qt::Desktop Architecture

## Overview

This section describes the Qt/C++ abstraction layer located between the central engine (`Core`) and the QML user interface. It manages communication logic, the worker thread, operation state, and image display.

## Key Components

### `ImageControllerBase` (and derivatives like `ImageControllerSGS`, `ImageControllerRHI`, `ImageControllerPainted`)
*   **Role:** Main orchestrator. Manages communication between QML, `PhotoEngine` (Core), `OperationStateManager`, and `DisplayManager`. Each derivative (`SGS`, `RHI`, `Painted`) handles a specific rendering path.
*   **Responsibilities:**
    *   Create and manage `PhotoEngine`, `DisplayManager`, `OperationStateManager`, `OperationModelManager`.
    *   Manage a worker thread for non-blocking operations (loading, applying operations).
    *   Connect `valueChanged` signals from operation models (via `OperationModelManager`) to `OperationStateManager`.
    *   Expose QML slots (like `loadImage`, `applyOperations`) and emit QML signals for results.
    *   Interact with `DisplayManager` for display updates.

### `OperationModelManager`
*   **Role:** Centralized management of creation, storage, and QML registration of operation models (like `BrightnessModel`, `ContrastModel`).
*   **Responsibilities:**
    *   Create instances of operation models.
    *   Provide typed lists (e.g., `std::vector<std::shared_ptr<BaseAdjustmentModel>>`) for efficient use.
    *   Register models into the QML context (e.g., `brightnessControl`, `contrastControl`).

### `OperationStateManager`
*   **Role:** Management of the **cumulative** state of active operations.
*   **Responsibilities:**
    *   Maintain the full list of active operations (as `OperationDescriptor`).
    *   Provide thread-safe methods to add, update, remove operations in this state.
    *   Provide a method to retrieve the full list of active operations.
    *   **Does not communicate directly** with `PhotoEngine`.

### `BaseAdjustmentModel` (and derivatives like `BrightnessModel`)
*   **Role:** C++ models for single-parameter adjustable operations (e.g., brightness, contrast).
*   **Responsibilities:**
    *   Inherit from `OperationProvider` for Qt infrastructure.
    *   Expose QML properties (`value`, `minimum`, `maximum`, `name`, `active`).
    *   Implement `IOperationModel` to provide an `OperationDescriptor`.
    *   Emit the `valueChanged` signal when the value is modified via `setValue`.

### `DisplayManager`
*   **Role:** Manage display-side logic: downsampling, zoom, pan, tile updates.
*   **Responsibilities:**
    *   Downsample the high-resolution image from `PhotoEngine` for display.
    *   Manage zoom and pan state.
    *   Send the downsampled image to the appropriate Qt Quick rendering item (`RHIImageItem`, `SGSImageItem`, etc.).

### Rendering Components (`IRenderingItemBase`, `BaseImageItem`, `RHIImageItem`, `SGSImageItem`, `PaintedImageItem`)
*   **Role:** Qt Quick items responsible for the final display of the image on screen.
*   **Responsibilities:**
    *   `IRenderingItemBase`: Abstract interface defining common methods (`setImage`, `updateTile`, `setZoom`, `setPan`) and state (`m_image_width`, `m_image_height`, `m_zoom`, `m_pan`, `m_image_mutex`).
    *   `BaseImageItem`: Provides common implementations for `imageWidth`, `imageHeight`, and `isImageValid`, inheriting from `IRenderingItemBase`.
    *   `RHIImageItem`: Uses `QQuickRhiItem` to leverage QRhi (Vulkan, Metal, DirectX12). Integrates `RHIImageNode` for direct manipulation of the render pipeline.
    *   `SGSImageItem`: Uses `QQuickItem` and `QSGSimpleTextureNode` via the Scene Graph.
    *   `PaintedImageItem`: Uses `QQuickPaintedItem` and `QPainter`.
    *   All concrete items inherit from their specific Qt Quick base (`QQuickRhiItem`, `QQuickItem`, `QQuickPaintedItem`) and from `BaseImageItem` to get common state and logic.
    *   Receive the updated image from `DisplayManager` and display it.
    *   Implement zoom and pan logic.

### `QmlContextSetup`
*   **Role:** Entry point for QML context configuration.
*   **Responsibilities:**
    *   Create `ImageControllerBase`.
    *   Call `OperationModelManager::registerModelsToQml` (via `ImageControllerBase`'s getter).
    *   Register `ImageControllerBase` in the QML context (as `controller`).

## QML Components

The QML layer provides the user interface for the application. Key aspects include:

*   **Operation Widgets:** Individual QML components (e.g., `BrightnessOperation.qml`) bind to C++ operation models (e.g., `brightnessControl`) and trigger updates via `setValue`.
*   **Panels:** Collapsible sections (e.g., `TonePanel.qml`) group related operation widgets.
*   **Display Area:** The central panel hosts the Qt Quick rendering item (e.g., `QMLSGSImageItem`) which displays the processed image.
*   **Context Setup:** `QmlContextSetup` ensures C++ objects (`controller`, operation models) are available in the QML context for binding.

## Main Flow

1.  **Initialization:** `QmlContextSetup` creates `ImageControllerBase`, which creates its dependencies (`OperationStateManager`, `OperationModelManager`, etc.). `OperationModelManager` creates the models, and `ImageControllerBase` connects them to `OperationStateManager`. Then, `QmlContextSetup` registers the models to QML via `OperationModelManager`.
2.  **QML Interaction:** A user moves a slider (e.g., Brightness). This calls `brightnessControl.setValue(newValue)` in QML.
3.  **Model Update:** `BaseAdjustmentModel::setValue` updates `m_params.value` and emits `valueChanged(newValue)`.
4.  **Connection and State Update:** The lambda in `ImageControllerBase::connectModelsToStateManager` connected to `valueChanged` is executed. It calls `OperationStateManager::addOrUpdateOperation(brightness_descriptor)`.
5.  **Operation Application:** The lambda retrieves the full list of active operations via `OperationStateManager::getActiveOperations()` and calls `QMetaObject::invokeMethod(ImageControllerBase::doApplyOperations, ...)` on the worker thread.
6.  **Core Call:** `ImageControllerBase::doApplyOperations` calls `PhotoEngine::applyOperations(full_list_of_operations)`.
7.  **Processing in Core:** `PhotoEngine` delegates to `StateImageManager` (Core) which applies the list of operations via `OperationPipeline` and updates the working image.
8.  **Display Update:** `ImageControllerBase` retrieves the new working image from `PhotoEngine` and passes it to `DisplayManager`, which downsamples it and sends it to the Qt Quick rendering item for display.