# Core Architecture Design Principles

The CaptureMoment core library is designed with modularity, high performance, and future extensibility in mind, which is essential for a modern, tile-based image processing engine. This document outlines the key key architectural decisions, focusing on how we separate data from behavior and use established design patterns.

---

## 1. Data-Centric Design: Plain Old Data (POD) / Value Types

A fundamental principle of this architecture is the segregation of data storage from processing logic. Core data structures are kept simple and easy to handle:

### Key Data Structures:

* **`CaptureMoment::Core::Common::ImageRegion` (formerly `ImageRegion`):** This structure represents a self-contained tile of pixel data (e.g., RGBA F32), along with its spatial coordinates.

  * **Why Value Type?** By making `ImageRegion` a simple container, it becomes highly efficient for memory management. It can be easily copied, moved, and passed between functions, minimizing complexity and supporting future integration with low-level processing frameworks (like Halide or CUDA) that thrive on raw, contiguous data buffers.

* **`CaptureMoment::Core::Operations::OperationDescriptor` (formerly `OperationDescriptor`):** This structure holds all necessary parameters to define a single processing step (e.g., brightness value, contrast factor, enable state).

  * **Why Value Type?** It acts as a configuration snapshot. The `OperationPipeline` uses this descriptor to instruct the `OperationFactory` what to create and what parameters to apply, keeping the engine itself free from hard-coded operation logic.

---

## 2. Manager and Engine Separation

The core functionality is split into specialized components: Managers handle resources and external concerns, while the Engine handles orchestration.

### `CaptureMoment::Core::Managers::ISourceManager` & `CaptureMoment::Core::Managers::SourceManager` (Strategy Pattern)

The `ISourceManager` defines the contract for handling the image source, namely, the crucial READ (`getTile`) and WRITE (`setTile`) operations.

* **Responsibility:** Encapsulate file I/O, caching, and physical pixel access.
* **Decoupling:** By using the `ISourceManager` interface, the core logic (the `OperationPipeline`) is entirely agnostic to the underlying I/O technology.
* **Implementation (`SourceManager`):** The concrete implementation uses industry-standard tools like OpenImageIO (OIIO) to handle diverse file formats and efficient caching via `OIIO::ImageBuf` and `OIIO::ImageCache`. This shields the rest of the application from OIIO's specific complexities.

---

## 3. Task-Based Processing Architecture: Abstraction and Orchestration

To manage the processing workflow effectively, especially in a potentially concurrent or sequential context, we introduce a layer of abstraction using interfaces and a central orchestrator.

### `CaptureMoment::Core::Domain::IProcessingTask` & `CaptureMoment::Core::Engine::PhotoTask` (Command Pattern / Task Abstraction)

The **`IProcessingTask`** interface defines a unit of work encapsulating the processing of an image region with a specific sequence of operations.

* **Responsibility:** Encapsulate the data (input tile, operations, factory), the execution logic (`execute`), and provide access to the result (`result`) and progress (`progress`).
* **Decoupling:** The interface abstracts the *how* of processing, allowing for different implementations (e.g., CPU-based, GPU-based in the future) or different types of tasks (e.g., loading, saving).
* **Implementation (`PhotoTask`):** A concrete implementation that applies a sequence of operations defined by `OperationDescriptors` to an `ImageRegion` using the static `OperationPipeline::applyOperations` method. It receives an `std::shared_ptr<Common::ImageRegion>` as input, processes it in-place, and stores the result internally.

### `CaptureMoment::Core::Domain::IProcessingBackend` & `CaptureMoment::Core::Engine::PhotoEngine` (Orchestrator / Facade)

The **`IProcessingBackend`** interface defines the contract for creating and submitting **`IProcessingTasks`**.

* **Responsibility:** Orchestrate the overall processing flow. This includes managing the state of the loaded image (`SourceManager`), maintaining the list of active operations, creating **`IProcessingTasks`** via `createTask`, and handling their execution via `submit`. It also manages committing the final result back to the source via `commitResult`. A crucial aspect is the management of the **`m_working_image`**, a copy of the original image held by `SourceManager`, which is modified by operations before being potentially committed back.
* **Implementation (`PhotoEngine`):** The main concrete orchestrator. It uses the `SourceManager` to load images and provide initial data (`tiles`). It creates `PhotoTasks` via `createTask` by first calling `SourceManager::getTile` (which returns an `std::unique_ptr<Common::ImageRegion>`), converting it to `std::shared_ptr<Common::ImageRegion>`, and then passing it to `std::make_shared<PhotoTask>`. It executes tasks (synchronously in v1 via `submit`) and provides methods like `commitResult` to write the processed tile (retrieved via `task->result()`) back to the `m_working_image`, and `commitWorkingImageToSource` to write the final `m_working_image` back to the `SourceManager` using `SourceManager::setTile`.

* **Benefit:** This centralizes the high-level logic (when to process, what operations are active, how to handle results, managing the working state) away from the low-level pipeline execution and resource management.

---

## 4. Low-Level Pipeline Execution: Statelessness and Utility

The core logic for applying a sequence of operations to a data unit is encapsulated in a stateless utility located within the `operation` directory.

### `CaptureMoment::Core::Operations::OperationPipeline` (Stateless Utility)

The `OperationPipeline` class (formerly `PipelineEngine`) is refactored into a stateless class containing only a static method (`apply` or `applyOperations`).

* **Responsibility:** Execute a sequence of operations on a given `ImageRegion`. It iterates through the `OperationDescriptor`s, uses the `OperationFactory` to create instances of the required operations, and executes them sequentially on the provided tile.
* **Decoupling:** It is independent of `SourceManager`, `PhotoEngine`, or any task management. It only knows about `ImageRegion`, `OperationDescriptor`, and `OperationFactory`.
* **Location:** This class resides in the `operations` folder, centralizing the low-level processing logic within the operation domain.

* **Benefit:** High reusability and testability. It's a pure function-like component that performs the core processing step. It also allows for `[[nodiscard]]` attribute on its return value (`bool`) to ensure callers check for success/failure.

---

## 5. Operation Management: The Factory Pattern

This pattern remains crucial for creating operation instances within the processing pipeline.

### Components

* **`CaptureMoment::Core::Operations::IOperation` (formerly `IOperation`):** The base interface for all operations (e.g., `OperationBrightness`, `OperationContrast`). It defines a single `execute(Common::ImageRegion& tile, const OperationDescriptor& descriptor)` method.

  * **Benefit:** Polymorphism. The `OperationPipeline` can treat every operation the same way, regardless of whether it performs a simple brightness adjustment or a complex convolution.

* **`CaptureMoment::Core::Operations::OperationFactory` (formerly `OperationFactory`):** This component is responsible for knowing how to construct concrete implementations of `IOperation` based on an `OperationType` defined in the `OperationDescriptor`.

  * **Benefit:** Adding a new operation (e.g., `OperationSaturation`) only requires defining the new class and registering it in the factory setup, without modifying the `OperationPipeline`'s core logic. This promotes high maintainability and scalability.

---

## 6. Serialization and Persistence: Interfaces and Strategies (Independent Layer)

The core library includes a flexible system for saving and loading the state of image operations using XMP metadata. This system is designed as an **independent layer**, separate from the core image processing engine (`PhotoEngine`), to maximize modularity and flexibility.

### Components

* **`CaptureMoment::Core::Serializer::IXmpProvider`:** An interface abstracting the low-level XMP packet read/write operations. This allows switching between different XMP libraries (e.g., Exiv2, Adobe XMP Toolkit) without changing dependent code.
  * **Implementation (`Exiv2Provider`):** A concrete implementation using the Exiv2 library to interact with XMP packets within image files.

* **`CaptureMoment::Core::Serializer::IXmpPathStrategy`:** An interface defining how to determine the file path for the XMP data associated with a given image path. This allows for different storage strategies (sidecar files, AppData directory, configurable path).
  * **Implementations (`SidecarXmpPathStrategy`, `AppDataXmpPathStrategy`, `ConfigurableXmpPathStrategy`):** Concrete implementations of the path strategy interface, each defining its own logic for mapping an image path to an XMP file path.

* **`CaptureMoment::Core::Serializer::IFileSerializerWriter` & `CaptureMoment::Core::Serializer::IFileSerializerReader`:** Interfaces for writing and reading operation data to/from a file format (currently XMP). They depend on `IXmpProvider` and `IXmpPathStrategy`.
  * **Implementations (`FileSerializerWriter`, `FileSerializerReader`):** Concrete implementations that use the injected provider and strategy to perform the actual serialization/deserialization of `OperationDescriptor` lists to/from XMP packets.

* **`CaptureMoment::Core::Serializer::FileSerializerManager`:** A high-level manager that orchestrates the `IFileSerializerWriter` and `IFileSerializerReader`. It provides a unified interface (`saveToFile`, `loadFromFile`) for **external UI/QML layers** to use, not directly managed by `PhotoEngine`.

* **`CaptureMoment::Core::Serializer::OperationSerialization`:** A namespace containing utility functions (`serializeParameter`, `deserializeParameter`) for converting `std::any` parameter values within `OperationDescriptor` to/from string representations suitable for storage in XMP metadata, preserving type information.

* **`CaptureMoment::Core::Serializer::Exiv2Initializer`:** A utility class ensuring the Exiv2 library is initialized before any operations are performed.

### Benefits of Independence

* **Modularity:** `PhotoEngine` focuses purely on image processing orchestration. The serialization logic is completely separate.
* **Flexibility:** The serialization layer (`FileSerializerManager`) can be managed and invoked independently by the UI layer (e.g., via `UISerializerManager` in the Qt module) without requiring `PhotoEngine` to hold a reference to it.
* **Maintainability:** Changes to serialization mechanisms or strategies do not impact the core processing engine.
* **Clear Responsibility:** `PhotoEngine` handles image processing state and pipeline execution. A separate service handles persistence.

* [🟦 **SEE SERIALIZER.md**](SERIALIZER.md).
---

## 7. Namespace Organization

The codebase is structured using a clear namespace hierarchy to improve modularity and maintainability:

* **`CaptureMoment::Core::Common`:** Contains fundamental data structures like `ImageRegion` and `PixelFormat`.
* **`CaptureMoment::Core::Operations`:** Contains operation-related logic, including `IOperation`, `OperationDescriptor`, `OperationFactory`, `OperationPipeline`, and specific operation implementations (e.g., `OperationBrightness`).
* **`CaptureMoment::Core::Managers`:** Contains managers responsible for resource handling, such as `ISourceManager` and `SourceManager`.
* **`CaptureMoment::Core::Domain`:** Contains domain-specific interfaces, such as `IProcessingTask` and `IProcessingBackend`.
* **`CaptureMoment::Core::Engine`:** Contains the core application logic orchestrators, such as `PhotoTask` and `PhotoEngine`.
* **`CaptureMoment::Core::Serializer`:** Contains serialization-related interfaces, implementations, and utilities (e.g., `IXmpProvider`, `FileSerializerWriter`, `OperationSerialization`).

This organization clarifies the role of each component and prevents naming collisions.

---

## Operations
* [🟦 **Operations**](OPERATIONS.md).