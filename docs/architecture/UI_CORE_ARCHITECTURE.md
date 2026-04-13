# 🧱 UI Core Architecture

## 🧱 Overview

This architecture manages the Qt/C++ abstraction layer located between the central engine (`Core`) and the QML user interface. It manages communication logic, the worker thread, operation state, image display, and acts as a bridge for file-based serialization/deserialization of operations initiated from the UI layer.

**Main Objective:** Provide a stable and responsive Qt/C++ backend for the QML UI, orchestrating communication with the Core processing engine and managing UI-specific state and services.

---

## 🧱 Key Components

### 1. `ImageControllerBase` (and derivatives like `ImageControllerSGS`, `ImageControllerRHI`, `ImageControllerPainted`)

*   **Role:** Main orchestrator. Manages communication between QML, `PhotoEngine` (Core), `OperationStateManager`, `DisplayManager`, and `SerializerController`. Each derivative (`SGS`, `RHI`, `Painted`) handles a specific rendering path.
*   **Responsibilities:**
    *   Create and manage `PhotoEngine`, `DisplayManager`, `OperationStateManager`, `OperationModelManager`.
    *   Manage a worker thread for non-blocking operations (loading, applying operations).
    *   Connect `valueChanged` signals from operation models (via `OperationModelManager`) to `OperationStateManager`.
    *   Expose QML slots (like `loadImage`, `applyOperations`) and emit QML signals for results.
    *   Interact with `DisplayManager` for display updates.
    *   **Does not directly manage serialization.**
    *   **Post-commit `26bf511`:** Calls `m_display_manager->setSourceImageSize(...)` after loading the source image. Uses `m_engine->getDownsampledDisplayImage(target_width, target_height)` to obtain the display-ready image directly from the Core, and passes it (via `std::move`) to `m_display_manager->createDisplayImage(...)` or `m_display_manager->updateDisplayTile(...)`.
    *   **Post-commit `features/viewport`:** Modified `doLoadImage` and `doApplyOperations` to retrieve the target *downsampled* size using `m_display_manager->downsampleSize()` instead of `m_display_manager->displayImageSize()`. In `doApplyOperations`, the call to `m_display_manager` was changed from `updateDisplayTile(...)` to `createDisplayImage(...)` after receiving the new downsampled image from the Core.

### 2. `OperationModelManager`

*   **Role:** Centralized management of creation, storage, and QML registration of operation models (like `BrightnessModel`, `ContrastModel`).
*   **Responsibilities:**
    *   Create instances of operation models.
    *   Provide typed lists (e.g., `std::vector<std::shared_ptr<BaseAdjustmentModel>>`) for efficient use.
    *   Register models into the QML context (e.g., `brightnessControl`, `contrastControl`).

### 3. `OperationStateManager`

*   **Role:** Management of the **cumulative** state of active operations.
*   **Responsibilities:**
    *   Maintain the full list of active operations (as `OperationDescriptor`).
    *   Provide thread-safe methods to add, update, remove operations in this state.
    *   Provide a method to retrieve the full list of active operations.
    *   **Does not communicate directly** with `PhotoEngine` or handle serialization.

### 4. `BaseAdjustmentModel` (and derivatives like `BrightnessModel`)

*   **Role:** C++ models for single-parameter adjustable operations (e.g., brightness, contrast).
*   **Responsibilities:**
    *   Inherit from `OperationProvider` for Qt infrastructure.
    *   Expose QML properties (`value`, `minimum`, `maximum`, `name`, `active`).
    *   Implement `IOperationModel` to provide an `OperationDescriptor`.
    *   Emit the `valueChanged` signal when the value is modified via `setValue`.

### 5. `DisplayManager`

*   **Role:** Manage display-side logic: viewport-aware downsampling, zoom, pan, tile updates.
*   **Responsibilities:**
    *   Managed its own downsampling logic using OIIO, held a reference to the source image, and calculated display size.
    *   **No longer** performs local downsampling. Delegates this responsibility entirely to the Core (`IWorkingImageHardware::downsample`). Manages zoom and pan state. Sends display-ready images (received from `ImageControllerBase`) to the appropriate Qt Quick rendering item (`RHIImageItem`, `SGSImageItem`, etc.). Notifies the Core layer of the source image size via `setSourceImageSize`. Implements the `fitToView` logic using the *displayed* image size (`m_display_image_size`) rather than just the source size.
    *   **Signature Change:** Methods `createDisplayImage` and `updateDisplayTile` now accept `std::unique_ptr<Core::Common::ImageRegion>`.
    *   **Post-commit `features/viewport`:** Integrates `ViewportManager` (`m_viewport_manager`). The `initialize` method is added to configure the `ViewportManager` with screen and viewport details. The `setSourceImageSize` method now uses `m_viewport_manager->calculateDisplay(...)` to determine the required *downsampled* size, emitting `displayImageRequest` if downsampling is needed. The `downsampleSize` and `maxDownsample` are exposed as QML properties. The `fitToView` and `setViewportSize` methods utilize calculations from the `ViewportManager`. A new method `setQualityMargin` is added to control the quality vs. performance trade-off via the `ViewportManager`.
    *   [🟦 **SEE VIEWPORT_MANAGEMENT.md**](ui/VIEWPORT_MANAGEMENT.md). 

### 6. `SerializerController`

*   **Role:** UI-layer manager for handling file-based serialization and deserialization of image operations using XMP metadata.
*   **Responsibilities:**
    *   Acts as a Qt/QML-friendly wrapper around the core `FileSerializerManager`.
    *   Provides Qt slots (`saveOperations`, `loadOperations`) callable from QML.
    *   Emits Qt signals (`operationsSaved`, `operationsLoaded`, `operationsLoadFailed`, etc.) to notify the UI layer about the outcome of serialization operations.
    *   **Does not handle the core logic** of serialization or manage the image processing state.
    *   **Does not communicate directly** with `PhotoEngine`.

### 7. Rendering Components (`IRenderingItemBase`, `BaseImageItem`, `RHIImageItem`, `SGSImageItem`, `PaintedImageItem`)

*   **Role:** Qt Quick items responsible for the final display of the image on screen.
*   **Responsibilities:**
    *   `IRenderingItemBase`: Abstract interface defining common methods (`setImage`, `updateTile`, `setZoom`, `setPan`) and state. **Post-commit `533b85c`:** No longer holds state members like `m_full_image`, `m_zoom`, `m_pan`, etc. Signature of `setImage` and `updateTile` changed to accept `std::unique_ptr<Core::Common::ImageRegion>`. `getFullImage()` now returns a raw pointer `const ImageRegion*`. `zoom()`, `pan()`, `getImageMutex()` are now pure virtual.
    *   `BaseImageItem`: Provides common implementations for `imageWidth`, `imageHeight`, `isImageValid`, `zoom()`, `pan()`, `getImageMutex()`, `getFullImage()`, inheriting from `IRenderingItemBase`. **Post-commit `058c558`:** Holds common state members (`m_full_image`, `m_image_mutex`, `m_zoom`, `m_pan`) as `protected`. Implements virtual handlers `onZoomChanged`, `onPanChanged`, `onImageChanged` for derived classes.
    *   `RHIImageItem`: Uses `QQuickRhiItem` to leverage QRhi (Vulkan, Metal, DirectX12). Integrates `RHIImageNode` for direct manipulation of the render pipeline. **Post-commits `8cb418e`/`1314bd1`:** Signatures updated for `std::unique_ptr`. Implements `onZoomChanged`, `onPanChanged`, `onImageChanged` to emit signals and trigger updates. `RHIImageItemRenderer` is declared as a `friend`.
    *   `SGSImageItem`: Uses `QQuickItem` and `QSGSimpleTextureNode` via the Scene Graph. **Post-commit `45f2117`:** Signatures updated for `std::unique_ptr`. Implements `onZoomChanged`, `onPanChanged`, `onImageChanged`. Optimized tile update logic. **Post-commit `features/raws` (`0db5f909`):** When updating the paint node, the internal linear F32 image data (from `m_full_image`) is interpreted using `QImage::Format_RGBA32FPx4`, assigned the `QColorSpace::SRgbLinear` color space, and then converted to `QImage::Format_RGBA8888` with the standard `QColorSpace::SRgb` for display, leveraging Qt's color space conversion utilities.
    *   `PaintedImageItem`: Uses `QQuickPaintedItem` and `QPainter`. **Post-commit `7bebceb`:** Signatures updated for `std::unique_ptr`. Implements `onZoomChanged`, `onPanChanged`, `onImageChanged`. Optimized tile update logic (treated as full replacement). **Post-commit `features/raws` (`106b57d6`):** The `convertImageRegionToQImage` utility method now interprets the linear F32 image data (from `ImageRegion`) using `QImage::Format_RGBA32FPx4`, assigns the `QColorSpace::SRgbLinear` color space, and then converts it to `QImage::Format_RGBA8888` with the standard `QColorSpace::SRgb` for drawing with `QPainter`, leveraging Qt's color space conversion utilities.
    *   All concrete items inherit from their specific Qt Quick base (`QQuickRhiItem`, `QQuickItem`, `QQuickPaintedItem`) and from `BaseImageItem` to get common state and logic.
    *   Receive the updated image from `DisplayManager` and display it.
    *   Implement zoom and pan logic.

### 8. `QmlContextSetup`

*   **Role:** Entry point for QML context configuration.
*   **Responsibilities:**
    *   Create `ImageControllerBase`.
    *   Optionally receive and hold an instance of `SerializerController` (typically created and injected by a higher-level application module).
    *   Call `OperationModelManager::registerModelsToQml` (via `ImageControllerBase`'s getter).
    *   Register `ImageControllerBase` and `SerializerController` in the QML context (as `controller` and `serializerController` respectively).

---

## 🔗 Dependencies

*   **Core Library:** `core` (for `PhotoEngine`, `FileSerializerManager`, `OperationDescriptor`, etc.).
*   **Qt Libraries:** `Qt6::Core`, `Qt6::Quick`, `Qt6::Gui`, `Qt6::GuiPrivate`, `Qt6::ShaderTools`.
*   **UI Desktop Layer:** `ui_desktop` (or `ui_qml`) depends on this `ui_core` library.

---

## 🧩 Flow Diagrams

### A. Image Processing Flow (updated for hardware abstraction, fused pipelines, viewport management, and Qt color space handling, post `features/raws`)

1.  **Initialization:** `QmlContextSetup` creates `ImageControllerBase`, which creates its dependencies (`OperationStateManager`, `OperationModelManager`, `DisplayManager`, etc.). `OperationModelManager` creates the models, and `ImageControllerBase` connects them to `OperationStateManager`. `DisplayManager` is initialized (e.g., via `DisplayManager::initialize`) with viewport and screen information, which configures its internal `ViewportManager`.
2.  **QML Interaction:** A user moves a slider (e.g., Brightness). This calls `brightnessControl.setValue(newValue)` in QML.
3.  **Model Update:** `BaseAdjustmentModel::setValue` updates `m_params.value` and emits `valueChanged(newValue)`.
4.  **Connection and State Update:** The lambda in `ImageControllerBase::connectModelsToStateManager` connected to `valueChanged` is executed. It calls `OperationStateManager::addOrUpdateOperation(brightness_descriptor)`.
5.  **Operation Application:** The lambda retrieves the full list of active operations via `OperationStateManager::getActiveOperations()` and calls `QMetaObject::invokeMethod(ImageControllerBase::doApplyOperations, ...)` on the worker thread.
6.  **Core Call:** `ImageControllerBase::doApplyOperations` calls `PhotoEngine::applyOperations(full_list_of_operations)`.
7.  **Processing in Core:** `PhotoEngine` delegates to `StateImageManager` (Core) which uses `OperationPipelineBuilder` to create an `OperationPipelineExecutor`. This executor applies the list of operations via **fused Halide pipeline execution** using the hardware-agnostic `IWorkingImageHardware` abstraction (`WorkingImageCPU_Halide` or `WorkingImageGPU_Halide`). The dynamic input buffer binding ensures the correct image data is processed by the fused pipeline.
8.  **Display Update (Core -> UI):** `ImageControllerBase::doApplyOperations` (or `doLoadImage`) calls `m_engine->getDownsampledDisplayImage(target_width, target_height)`. The `target_width` and `target_height` are now obtained from `m_display_manager->downsampleSize()`, which was calculated by the `DisplayManager`'s `ViewportManager` based on the source image size and viewport characteristics.
9.  **Display Update (UI Propagation):** `ImageControllerBase` receives the downsampled image (as `std::unique_ptr<Core::Common::ImageRegion`) and passes it (via `std::move`) to `m_display_manager->createDisplayImage(...)` (or `updateDisplayTile(...)` depending on the flow, though `createDisplayImage` is used in `doApplyOperations` as per the latest changes).
10. **Display Update (UI Rendering):** `DisplayManager` receives the image and passes it (via `std::move`) to the Qt Quick rendering item (e.g., `RHIImageItem::setImage`) for display. **Post-commit `features/raws`:** Items like `SGSImageItem` and `PaintedImageItem` now use Qt's `QColorSpace` utilities to correctly convert the linear F32 image data received from the Core to the standard sRGB color space expected for display on screen.

### B. Serialization Flow (unchanged)
1.  **Initialization:** `QmlContextSetup` receives an instance of `SerializerController` (created elsewhere) and registers it to the QML context as `serializerController`.
2.  **QML Interaction:** A user triggers a save/load action (e.g., clicking a "Save XMP" button). QML calls `serializerController.saveOperations(imagePath, operationList)` or `serializerController.loadOperations(imagePath)`.
3.  **Serialization Call:** `SerializerController` receives the call and delegates the work to the underlying core `FileSerializerManager`.
4.  **Signal Notification:** Upon completion, `SerializerController` emits a Qt signal (e.g., `operationsSaved`, `operationsLoaded(loadedOperations)`, `operationsLoadFailed`).
5.  **QML Response:** QML listens to these signals and updates the UI accordingly (e.g., show a success message, apply loaded operations to the `OperationStateManager`).

---
