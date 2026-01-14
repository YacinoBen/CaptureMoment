# 🏗️ Qt::Desktop Architecture (QML Layer)

## Overview

This section describes the QML user interface layer. It interacts with the Qt/C++ UI Core layer (`ImageControllerBase`, `Operation Models`, `SerializerController`) to provide the user experience for loading images, adjusting parameters, viewing results, and managing operation persistence.

## Key QML Components

### QML Operation Widgets

*   **Role:** Individual UI controls (e.g., sliders, checkboxes) for specific image adjustments (e.g., Brightness, Contrast).
*   **Interaction:** Bind to C++ operation models (e.g., `brightnessControl`, `contrastControl`) obtained from the QML context. Trigger updates via model setters (e.g., `model.setValue(newVal)`).

### QML Panels

*   **Role:** Organizational containers (e.g., `TonePanel.qml`) grouping related operation widgets together.

### QML Display Area

*   **Role:** Central area hosting the Qt Quick rendering item (e.g., `QMLSGSImageItem.qml`, `QMLRHIImageItem.qml`, `QMLPaintedImageItem.qml`) which displays the processed image received from the `DisplayManager`.

### QML Context Integration

*   **Role:** The QML layer receives C++ objects from `QmlContextSetup`.
*   **Objects Available:**
    *   `controller`: Instance of `ImageControllerBase`. Used for `loadImage`, `applyOperations`, accessing `operationStateManager`, etc.
    *   `serializerController`: Instance of `SerializerController`. Used for `saveOperations`, `loadOperations`, listening to `operationsSaved`, `operationsLoaded`, etc.
    *   Operation Models (e.g., `brightnessControl`, `contrastControl`).

## QML Interaction Flows

### Image Adjustment Flow

1.  **User Action:** User drags a slider in a QML Operation Widget (e.g., Brightness).
2.  **QML Binding:** The slider's `valueChanged` signal updates the bound C++ model's `value` property (e.g., `brightnessControl.value = newValue`).
3.  **C++ Model Signal:** The C++ `BaseAdjustmentModel` emits its `valueChanged` signal.
4.  **C++ Connection:** `ImageControllerBase` receives the signal and triggers the processing pipeline (see `ui_core_architecture.md`).

### Operation Persistence Flow

1.  **User Action:** User clicks a "Save Adjustments" or "Load Adjustments" button in QML.
2.  **QML Call:** QML calls a slot on the exposed `serializerController` object (e.g., `serializerController.saveOperations(controller.getCurrentImagePath(), controller.operationStateManager.getActiveOperations())` or `serializerController.loadOperations(controller.getCurrentImagePath())`).
3.  **C++ Serialization:** `SerializerController` handles the core serialization logic (see `ui_core_architecture.md`).
4.  **QML Feedback:** QML listens to signals emitted by `serializerController` (e.g., `onOperationsSaved`, `onOperationsLoadFailed`) and updates the UI (e.g., shows success/error messages).

## Main Flow (Combined UI + Core)

The primary flow involves QML triggering actions via `controller` or `serializerController`, which then propagate through the UI Core layer and eventually to the Core engine for processing or persistence.

---

## 🛠️ How to Contribute

### Adding a New QML Operation Widget

1.  **Create QML Component:** Define a new QML file (e.g., `VignetteOperation.qml`).
2.  **Bind to Model:** Use `Binding` or direct property binding to connect the QML control (slider, checkbox) to the corresponding C++ operation model property (e.g., `model: vignetteControl`).
3.  **Register Model:** Ensure the C++ `VignetteModel` is created and registered by `OperationModelManager` and exposed to QML (see `ui_core_architecture.md`).

### Adding a New QML Persistence Feature

1.  **Create UI Elements:** Add buttons, dialogs, or other QML components to trigger save/load actions.
2.  **Call Serializer Controller:** Use the `serializerController` object available in the QML context to call `saveOperations` or `loadOperations`.
3.  **Handle Signals:** Listen to signals from `serializerController` to provide user feedback.