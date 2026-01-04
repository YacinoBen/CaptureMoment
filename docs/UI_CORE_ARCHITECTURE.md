# 🧱 Architecture UI Core

## 🧱 Key Components

### 1. Controllers (`controller/`)

*   **Purpose:** Orchestrate the flow between the UI (QML) and the core engine (`PhotoEngine`, `SourceManager`, etc.).
*   **Responsibilities:**
    *   Expose core functionality to QML via `Q_PROPERTY`, `Q_INVOKABLE`, and signals/slots.
    *   Manage the worker thread for non-blocking image processing.
    *   Handle communication with rendering items (e.g., passing processed images).
    *   Manage the `DisplayManager`.
*   **Base Class:** `ImageControllerBase` (abstract, defines common interface and threading).
*   **Concrete Implementations:** `ImageControllerPainted`, `ImageControllerRHI`, `ImageControllerSGS` (handle specific rendering paths).
*   **Common Logic:** `doLoadImage` and `doApplyOperations` are now implemented **in `ImageControllerBase`** for commonality, reducing duplication.

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
    *   Provide `QQuickItem` subclasses (e.g., `QQuickPaintedItem`, `QSGRenderNode`, `QQuickRhiItem`).
    *   Interface with Qt's Rendering Hardware Interface (`QRhi`) for high-performance GPU rendering.
    *   Handle texture updates, geometry, and shader management.
    *   Implement zoom and pan transformations.
*   **Base Interface:** `IRenderingItemBase` (defines common methods like `setImage`, `updateTile`, common data members like `m_zoom`, `m_pan`, `m_image_width`, `m_image_height`, `m_image_mutex`).
*   **Base Implementation:** `BaseImageItem` (provides common implementations for `imageWidth()` and `imageHeight()` using `m_image_mutex` from `IRenderingItemBase`, and provides `isImageValid()` method).
*   **Concrete Items:** `RHIImageItem`, `PaintedImageItem`, `SGSImageItem`. These classes inherit from their specific Qt Quick base (`QQuickRhiItem` for RHI, `QQuickItem` for SGS, `QQuickPaintedItem` for Painted) and from `BaseImageItem` to get common data and logic. They implement the remaining methods of `IRenderingItemBase` (e.g., `setImage`, `updateTile`, `setZoom`, `setPan`) and declare their own QML properties and signals (`Q_PROPERTY`, `signals`).
*   **QML Wrapper Classes:** `QMLSGSImageItem`, `QMLPaintedImageItem`, `QMLRHIImageItem`. These classes inherit from the concrete rendering items and re-declare `Q_PROPERTY` for seamless QML binding.
*   **Node Implementation:**
    *   **RHI:** `RHIImageItem` now inherits from `QQuickRhiItem` and uses a **separate renderer class** `RHIImageItemRenderer` (implementing `QQuickRhiItemRenderer`) for QRhi rendering logic, separating item logic from render logic.
    *   **SGS:** Uses `QSGSimpleTextureNode` via `updatePaintNode`.
    *   **Painted:** Uses `QQuickPaintedItem` and `QPainter` via `paint`.

### 4. Operation Models (`models/operations/`)

*   **Purpose:** Expose specific image operation parameters and state to QML.
*   **Responsibilities:**
    *   Implement the `IOperationModel` interface.
    *   Expose properties (e.g., `value`, `minimum`, `maximum`, `name`, `active`) and methods (e.g., `setValue`) to QML.
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
9.  **Rendering -> GPU:** The rendering item (e.g., `RHIImageItem` via `RHIImageItemRenderer`, `SGSImageItem` via `updatePaintNode`, `PaintedImageItem` via `paint`) updates the GPU texture or QPainter image and renders the image via `QRhi` or `QPainter`.

**Important : RHI, SGS, and Painted are now working correctly with the new rendering architecture.**

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

1.  **Implement QQuickItem:** Create `VulkanImageItem` inheriting from `QQuickItem` (or `QQuickRhiItem` for new RHI-based approach, or `QSGRenderNode` or `QQuickPaintedItem`).
2.  **Integrate QRhi/Vulkan:** Implement `createRenderer` (for `QQuickRhiItem`) or `updatePaintNode` or `prepare`/`render` methods using `QRhi` for Vulkan/Metal/DirectX or `paint` for `QQuickPaintedItem`.
3.  **Inherit from Base:** `VulkanImageItem` should inherit from its specific Qt Quick base class (`QQuickItem`/`QQuickRhiItem`/`QQuickPaintedItem`) and from `BaseImageItem` to get common data members and implementations (`imageWidth`, `imageHeight`, `isImageValid`).
4.  **Implement IRenderingItemBase:** Implement the remaining pure virtual methods from `IRenderingItemBase` (e.g., `setImage`, `updateTile`, `setZoom`, `setPan`) in `VulkanImageItem`.
5.  **Declare QML Properties/Signals:** Declare `Q_PROPERTY` and `signals` in `VulkanImageItem` for QML exposure.
6.  **Create QML Wrapper:** Create `QMLVulkanImageItem` inheriting from `VulkanImageItem` and re-declaring `Q_PROPERTY`.
7.  **Integrate Controller:** Adapt existing controllers (e.g., `ImageControllerBase`) or create a new one (e.g., `ImageControllerVulkan`) to interact with the new item via `DisplayManager`.
8.  **Register/Integrate:** Ensure the new item can be used via `DisplayManager` and registered in `QmlContextSetup`.

### Adding a New Controller Path (e.g., ImageControllerVulkan)

1.  **Inherit Base:** Create `ImageControllerVulkan` inheriting from `ImageControllerBase`.
2.  **Override Virtuals:** Implement `doLoadImage` and `doApplyOperations` to handle the specific rendering path and interact with the corresponding rendering item (e.g., `VulkanImageItem`) via `DisplayManager`.
3.  **Integrate:** Ensure `QmlContextSetup` can create and configure this new controller type if needed.
