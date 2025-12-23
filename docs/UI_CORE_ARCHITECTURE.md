# 🏗️ Architecture - CaptureMoment Qt Core Library (`qt/core`)

## 🎯 Overview

The `qt/core` library (`capturemoment_qt_core_lib`) serves as the **Qt-specific bridge** between the **pure C++ core logic** (`core`) and the **QML/Qt Quick UI** (`qt/desktop`). It **exposes C++ classes to QML**, implements **Qt Quick rendering items** (using `QQuickItem`, `QSGRenderNode`, `QRhi`), and provides **controller classes** that orchestrate the interaction between UI and core logic in a **thread-safe manner**.

## 📁 Directory Structure
qt/core/

├── CMakeLists.txt # CMake configuration for the static library

├── include/

│ ├── controller/ # Controller classes (ImageControllerBase, ...)

│ ├── display/ # Display management (DisplayManager)

│ ├── rendering/ # Qt Quick rendering components (RHIImageItem, ...)

│ └── models/

│ │     └── operations/ # QML-exposed operation models (BrightnessModel, ...)


## 🧱 Key Components

### 1. Controllers (`controller/`)

*   **Purpose:** Orchestrate the flow between the UI (QML) and the core engine (`PhotoEngine`, `SourceManager`, etc.).
*   **Responsibilities:**
    *   Expose core functionality to QML via `Q_PROPERTY`, `Q_INVOKABLE`, and signals/slots.
    *   Manage the worker thread for non-blocking image processing.
    *   Handle communication with rendering items (e.g., passing processed images).
    *   Manage the `DisplayManager`.
*   **Base Class:** `ImageControllerBase` (abstract, defines common interface and threading).
*   **Concrete Implementations:** `ImageControllerPainted`, `ImageControllerRHI`, etc. (handle specific rendering paths).

### 2. Display Management (`display/`)

*   **Purpose:** Handle the **display-side logic** such as **downsampling**, **zoom**, **pan**, and **viewport management**.
*   **Responsibilities:**
    *   Downsample high-resolution images from the core for efficient display.
    *   Manage zoom and pan state.
    *   Coordinate with rendering items to display the processed/downsampled image.
    *   Provide coordinate mapping between backend (full-res) and display (downsampled) coordinates.
*   **Key Class:** `DisplayManager`.

### 3. Rendering (`rendering/`)

*   **Purpose:** Implement Qt Quick items responsible for **displaying images** on screen using various Qt Quick rendering techniques.
*   **Responsibilities:**
    *   Provide `QQuickItem` subclasses (e.g., `QQuickPaintedItem`, `QSGRenderNode`).
    *   Interface with Qt's Rendering Hardware Interface (`QRhi`) for high-performance GPU rendering.
    *   Handle texture updates, geometry, and shader management.
    *   Implement zoom and pan transformations.
*   **Base Interface:** `IRenderingItemBase` (defines common methods like `setImage`, `updateTile`).
*   **Concrete Items:** `RHIImageItem`, `PaintedImageItem`, `SGSImageItem`.
*   **Node Implementation:** `RHIImageNode` (used by `RHIImageItem` for QRhi rendering).

### 4. Operation Models (`models/operations/`)

*   **Purpose:** Expose specific image operation parameters and state to QML.
*   **Responsibilities:**
    *   Implement the `IOperationModel` interface.
    *   Expose properties (e.g., `value`, `minimum`, `maximum`, `active`) and methods (e.g., `setValue`) to QML.
    *   Communicate with the `ImageController` to trigger the application of the operation.
    *   Hold operation-specific parameters (`RelativeAdjustmentParams`, etc.).
*   **Base Class:** `OperationProvider` (provides common Qt infrastructure).
*   **Example:** `BrightnessModel`.

### 5. Shaders (`shaders/glsl/`)

*   **Purpose:** GLSL source code for custom GPU effects used by rendering components (e.g., `RHIImageItem`).
*   **Management:** Compiled automatically into `.qsb` files by `qt_add_shaders` in `CMakeLists.txt`.

## 🔗 Dependencies

*   **Core Library (`capturemoment_core`):** `qt/core` **depends** on the core library for image processing logic (`PhotoEngine`, `SourceManager`, `OperationPipeline`, etc.).
*   **Qt Libraries:** `Qt6::Core`, `Qt6::Quick`, `Qt6::Gui`, `Qt6::GuiPrivate`, `Qt6::ShaderTools`.
*   **QML Integration:** `QmlContextSetup` (in `qt/desktop`) links objects from `qt/core` to the QML context.

## 🧩 How Components Work Together

1.  **QML Interaction:** User interacts with a control (e.g., brightness slider) in QML.
2.  **QML -> C++:** The QML component (e.g., `BrightnessOperation.qml`) calls a method on a C++ model (e.g., `brightnessControl.setValue(...)`).
3.  **Model -> Controller:** The model (e.g., `BrightnessModel`) emits a signal or calls a method on the controller (e.g., `controller.applyOperations(...)`).
4.  **Controller -> Core:** The controller (`ImageControllerPainted`) queues the operation request to its worker thread, which then interacts with the core engine (`PhotoEngine`).
5.  **Core Processing:** The core engine (`PipelineEngine`, `OperationPipeline`) applies the operation to the image data.
6.  **Core -> Controller:** The core engine returns the processed image data (e.g., via `task->result()`).
7.  **Controller -> Display:** The controller passes the result to the `DisplayManager`.
8.  **Display -> Rendering:** The `DisplayManager` downsamples the image (if necessary) and sends it to the appropriate rendering item (e.g., `RHIImageItem`).
9.  **Rendering -> GPU:** The rendering item (e.g., `RHIImageItem` via `RHIImageNode`) updates the GPU texture and renders the image via `QRhi` or `QPainter`.

**Important : RHI and SGS are not working now**

## 🛠️ How to Contribute

### Adding a New Operation Model (e.g., ContrastModel)

1.  **Create C++ Model:**
    *   Define `ContrastModel` inheriting from `OperationProvider` (or implementing `IOperationModel` directly).
    *   Declare `Q_PROPERTY` for `value`, `minimum`, `maximum`, `name`, `active`.
    *   Implement `Q_INVOKABLE setValue(real value)` and emit `valueChanged`.
    *   Implement `getType()`, `getDescriptor()`, `isActive()`, etc.
    *   Store parameters using a struct like `RelativeAdjustmentParams`.
    *   Handle `setValue` by updating parameters, emitting signals, and triggering `ImageController::applyOperations`.
2.  **Register in C++:** Add `m_contrast_model` to `QmlContextSetup` and register it via `setContextProperty`.
3.  **Create QML Component:** Create `ContrastOperation.qml` in `qt/desktop/qml/components/operations/`, binding its slider/spinbox to `contrastControl.value` and calling `contrastControl.setValue` on change.
4.  **Add to Panel:** Include `ContrastOperation.qml` in the relevant panel (e.g., `TonePanel.qml`).

### Adding a New Rendering Item (e.g., VulkanImageItem)

1.  **Implement QQuickItem:** Create `VulkanImageItem` inheriting from `QQuickItem` (or `QSGRenderNode`).
2.  **Integrate QRhi/Vulkan:** Implement `updatePaintNode` or `prepare`/`render` methods using `QRhi` for Vulkan/Metal/DirectX.
3.  **Implement IRenderingItemBase:** Make it conform to the `IRenderingItemBase` interface (or adapt existing controllers).
4.  **Create Controller Path:** Add a new controller type (e.g., `ImageControllerVulkan`) if necessary, or adapt existing ones to work with the new item.
5.  **Register/Integrate:** Ensure the new item can be used via `DisplayManager`.

### Adding a New Controller Path (e.g., ImageControllerVulkan)

1.  **Inherit Base:** Create `ImageControllerVulkan` inheriting from `ImageControllerBase`.
2.  **Override Virtuals:** Implement `doLoadImage` and `doApplyOperations` to handle the specific rendering path.
3.  **Set Rendering Item:** In `doLoadImage`/`doApplyOperations`, ensure the processed image is sent to the correct `VulkanImageItem` via `DisplayManager`.
4.  **Integrate:** Ensure `QmlContextSetup` can create and configure this new controller type if needed.