# 🧱 Core Architecture

## 🧱 Overview

This architecture manages the central image processing logic, independent of the UI layer. It provides a robust, modular, and testable foundation for image loading, manipulation, and state management.

**Main Objective:** Provide a performant and extensible backend for image operations, handling image data, processing pipelines, and cumulative operation state.

---

## 🧱 Key Components

### 1. `PhotoEngine`

*   **Role:** Central orchestrator of core image processing.
*   **Responsibilities:**
    *   Manage the **original image source** via `SourceManager`.
    *   Delegate the application of operations to `StateImageManager`.
    *   Expose the current **working image** state to the UI layer (`ImageControllerBase`).
    *   Provide metadata (width, height) of the current working image.
    *   **Does NOT directly process operations.** It delegates to `StateImageManager`.
*   **Location:** `engine/`.

### 2. `StateImageManager`

*   **Role:** Manages the **cumulative state** of image operations and the **working image**.
*   **Responsibilities:**
    *   Store the **original image source** (from `SourceManager`).
    *   Maintain the **current working image** (result of applying active operations).
    *   Maintain the **list of active operations** (`std::vector<OperationDescriptor>`).
    *   Provide methods to **modify** the active operations list (`addOperation`, `removeOperation`, `resetToOriginal`).
    *   Provide a method to **request an update** (`requestUpdate`), which triggers the application of the full list of active operations via `OperationPipeline`.
    *   Execute the processing pipeline on a **separate thread** (`m_worker_thread`) to avoid blocking the caller (e.g., `ImageControllerBase`).
    *   Ensure **thread-safe access** to the original image source and the working image using synchronization mechanisms (`std::mutex`, `std::atomic`).
    *   **Does NOT know about UI layers.**
*   **Location:** `managers/`.

### 3. `SourceManager`

*   **Role:** Handles loading and providing access to the original image data.
*   **Responsibilities:**
    *   Load image files from disk into memory (e.g., using FreeImage, OpenCV, etc.).
    *   Store the loaded image data (e.g., as `std::vector<float>` for RGBA_F32).
    *   Provide **thread-safe** access to the original image data (e.g., `getTile`).
    *   Provide metadata (width, height, format) of the loaded image.
    *   **Does NOT process operations.**
*   **Location:** `managers/`.

### 4. `OperationPipeline`

*   **Role:** Executes a **sequence** of image operations on image data.
*   **Responsibilities:**
    *   Receive a list of `OperationDescriptor` and an input image buffer.
    *   Iterate through the list of operations.
    *   Use `OperationFactory` to instantiate each operation object.
    *   Execute each operation object's `execute` method in sequence, passing the image data between operations.
    *   Return the final processed image buffer.
    *   **Does NOT manage the list of active operations.** That's `StateImageManager`'s job.
    *   **Does NOT know about UI layers.**
*   **Location:** `operations/`.

### 5. `OperationFactory`

*   **Role:** Creates instances of specific image operation implementations based on their type.
*   **Responsibilities:**
    *   Register creators for different operation types (e.g., `Brightness`, `Contrast`, `Highlights`, `Shadows`, `Whites`, `Blacks`).
    *   Provide a method (e.g., `create`) to instantiate an operation object given an `OperationDescriptor`.
    *   Use the **Factory Pattern** for loose coupling.
    *   **Does NOT execute operations.**
*   **Location:** `operations/`.

### 6. `IOperation`

*   **Role:** Pure interface defining the contract for all image operations.
*   **Responsibilities:**
    *   Define the `execute` method signature that all operation implementations must follow.
    *   Ensure all operations can be executed by `OperationPipeline`.
*   **Location:** `operations/`.

### 7. Concrete Operation Classes (e.g., `OperationBrightness`, `OperationContrast`)

*   **Role:** Implement specific image processing algorithms (e.g., brightness adjustment, contrast adjustment).
*   **Responsibilities:**
    *   Inherit from `IOperation`.
    *   Implement the `execute` method using a processing library (e.g., Halide).
    *   Use parameters from the `OperationDescriptor` (e.g., brightness value, contrast value).
    *   **Do NOT manage the state of active operations.**
*   **Location:** `operations/impl/` (or similar subdirectory).

### 8. `OperationDescriptor`

*   **Role:** Data structure representing a single operation to be applied.
*   **Responsibilities:**
    *   Store the operation type (`OperationType`).
    *   Store the operation name (for logging/debugging).
    *   Store the operation parameters (e.g., `value`, `enabled` status).
    *   Store generic parameters using a map-like structure (e.g., `std::map<std::string, AnyValue>` or similar).
    *   Be passed between `StateImageManager`, `OperationPipeline`, and `OperationFactory`.
*   **Location:** `operations/`.

### 9. `OperationType`

*   **Role:** Enum defining the types of supported operations.
*   **Responsibilities:**
    *   Provide a unique identifier for each operation type (e.g., `Brightness`, `Contrast`).
    *   Be used by `OperationFactory` and `OperationDescriptor`.
*   **Location:** `operations/`.

### 10. `OperationRegistry`

*   **Role:** Utility to register all available operations with the `OperationFactory` at application startup.
*   **Responsibilities:**
    *   Iterate through known operation types.
    *   Call `factory->registerCreator` for each type, associating it with its concrete implementation.
    *   Ensure `OperationFactory` is ready to create any supported operation.
*   **Location:** `operations/`.

---

## 🔗 Dependencies

*   **Core Libraries:** Uses image loading libraries (e.g., FreeImage), potentially image processing libraries (e.g., OpenCV), and performance libraries (e.g., Halide).
*   **Qt Libraries:** `Qt6::Core` (for threading primitives like `QThread`, `QMetaObject::invokeMethod` if used for worker thread communication).
*   **UI Layer:** `qt/core` depends on this `core` library.

---

## 🧩 Flow Diagrams

### A. Image Loading Flow
PhotoEngine::loadImage(file_path)
├── SourceManager::loadFile(file_path)
│ └── Loads image data into memory
└── StateImageManager::setOriginalImageSource (from SourceManager)
└── Stores reference/pointer to original data
└── Resets working image to original state

### B. Operation Application Flow
PhotoEngine::applyOperations(std::vector ops)
├── StateImageManager::setOperations(ops)
│ ├── Acquires write lock on operation list
│ ├── Updates internal list of active operations
│ └── Releases lock
└── StateImageManager::requestUpdate()
├── QMetaObject::invokeMethod (on m_worker_thread)
│ └── Calls StateImageManager::performUpdate (on worker thread)
└── Returns immediately (async)
└── StateImageManager::performUpdate (on worker thread)
├── Acquires read lock on operation list
├── Acquires read lock on original image source
├── OperationPipeline::applyOperations (list_of_ops, original_image)
│ ├── Iterates list_of_ops
│ ├── OperationFactory::create (op_descriptor)
│ │ └── Instantiates specific operation (e.g., OperationBrightness)
│ ├── operation->execute (image_data)
│ │ └── Modifies image_data (e.g., using Halide)
│ └── (Repeats for all ops in list)
│ └── Returns final processed image_data
├── Acquires write lock on working image
├── Updates m_working_image with result
└── Releases lock
└── UI layer (ImageControllerBase) can now retrieve the updated working image


---

## 🛠️ How to Contribute

### Adding a New Operation (e.g., Vignette)

1.  **Define Operation Type:** Add `Vignette` to the `OperationType` enum.
2.  **Create Operation Implementation:**
    *   Define `OperationVignette` class.
    *   Inherit from `IOperation`.
    *   Implement `execute` method using a processing library (e.g., Halide) to apply the vignette effect based on parameters from the `OperationDescriptor`.
3.  **Register Operation:**
    *   Add `OperationVignette` to the `OperationRegistry::registerAll` function.
    *   Ensure `OperationFactory` knows how to create `OperationVignette`.
4.  **UI Integration:** Create a corresponding operation model in `qt/core` (`VignetteModel`) and expose it to QML.