# Core Architecture Design Principles
The CaptureMoment core library is designed with modularity, high performance, and future extensibility in mind, which is essential for a modern, tile-based image processing engine. This document outlines the key architectural decisions, focusing on how we separate data from behavior and use established design patterns.

---
## 1. Data-Centric Design: Plain Old Data (POD) / Value Types
A fundamental principle of this architecture is the segregation of data storage from processing logic. Core data structures are kept simple and easy to handle:
### Key Data Structures:
* **`CaptureMoment::Core::Common::ImageRegion` (formerly `ImageRegion`):** This structure represents a self-contained tile of pixel data (e.g., RGBA F32), along with its spatial coordinates.
* **Why Value Type?** By making `ImageRegion` a simple container, it becomes highly efficient for memory management. It can be easily copied, moved, and passed between functions, minimizing complexity and supporting future integration with low-level processing frameworks (like Halide or CUDA) that thrive on raw, contiguous data buffers.
* **Uniform Types:** The dimensions (`m_width`, `m_height`), coordinates (`m_x`, `m_y`), and channels (`m_channels`) now use common types defined in `Common::ImageDim`, `Common::ImageCoord`, and `Common::ImageChan` respectively for consistency across the codebase.
* **Constructors:** Includes constructors for direct initialization and move semantics (zero-copy).
* **`CaptureMoment::Core::Operations::OperationDescriptor` (formerly `OperationDescriptor`):** This structure holds all necessary parameters to define a single processing step (e.g., brightness value, contrast factor, enable state).
* **Why Value Type?** It acts as a configuration snapshot. The `OperationPipeline` uses this descriptor to instruct the `OperationFactory` what to create and what parameters to apply, keeping the engine itself free from hard-coded operation logic.
---
## 2. Manager and Engine Separation
The core functionality is split into specialized components: Managers handle resources and external concerns, while the Engine handles orchestration.
### `CaptureMoment::Core::Managers::ISourceManager` & `CaptureMoment::Core::Managers::SourceManager` (Strategy Pattern)
The `ISourceManager` defines the contract for handling the image source, namely, the crucial READ (`getTile`) and WRITE (`setTile`) operations.
* **Responsibility:** Encapsulate file I/O, caching access.
* **Decoupling:** By using the `ISourceManager` interface, the core logic (the `OperationPipeline`) is entirely agnostic to the underlying I/O technology.
* **Implementation (`SourceManager`):** The concrete implementation uses industry-standard tools like OpenImageIO (OIIO) to handle diverse file formats and efficient caching via `OIIO::ImageBuf` and `OIIO::ImageCache rest of the application from OIIO's specific complexities.
* **Pattern Explanation (Strategy):** The `ISourceManager` interface defines a family of algorithms (different I/O methods). `SourceManager` provides a concrete implementation. This allows switching between different I/O strategies (e.g., OIIO, native filesystem, network) without changing the code that depends on `ISourceManager`, promoting flexibility and maintainability.
* **Uniform Types:** Methods like `width()`, `height()`, `channels()` now return `Common::ImageDim`, `Common::ImageDim`, `Common::ImageChan` respectively.
---
## 3. Hardware-Agnostic Processing Architecture
The most significant recent evolution is the introduction of a hardware-agnostic processing layer that abstracts CPU and GPU execution behind a unified interface.
### `CaptureMoment::Core::ImageProcessing::IWorkingImageHardware` (Abstract Interface)
This interface represents an image used as a working buffer, abstracting its hardware location (CPU RAM or GPU memory).
* **Key Methods:**
* `exportToCPUCopy()`: Creates a CPU copy of the image data for display or saving.
* `updateFromCPU(const ImageRegion&)`: Updates the working buffer from CPU data.
* `isValid()`: Checks if the buffer is valid.
* `getSize()`, `getChannels()`, `getDataSize()`: Query buffer dimensions. These now return `Common::ImageDim`, `Common::ImageChan`, `Common::ImageSize` respectively.
* `downsample(size_t target_width, size_t target_height)`: Exports a downscaled version of the image directly from the hardware buffer, optimizing for display purposes. This method replaces the previous destructive `exportToCPUMove()` and is preferred for UI updates.
* **Pattern Explanation (Interface/Abstraction):** The interface `IWorkingImageHardware` defines the contract for interacting with an image buffer, regardless of its physical location. Concrete implementations (`WorkingImageCPU_Halide`, `WorkingImageGPU_Halide`) provide the specific logic for CPU or GPU memory. This allows the rest of the application (like `OperationPipelineExecutor`) to work with the abstract interface, promoting hardware independence and easier testing.
### Base Classes and Hierarchies
* **`WorkingImageData`**: Base class providing raw pixel data storage (`std::unique_ptr<float[]>`) and metadata (width, height, channels, validity state `m_valid`) for all working image implementations. Introduces `initializeData` for buffer setup and `getDataSpan` for safe access. Uses `Common::ImageDim`, `Common::ImageChan`, `Common::ImageSize` for its members.
* **`WorkingImageHalide`**: Base class providing shared Halide buffer (`Halide::Buffer<float>`) logic. Offers methods to initialize the buffer view (`initializeHalide` taking a `std::span`), and specific getters for dimensions/channels based on the Halide buffer (`getSizeByHalide`, `getChannelsByHalide`, etc.). These getters now return `Common::ImageDim`, `Common::ImageChan`, `Common::ImageSize`.
* **`WorkingImageCPU`**: Concrete base class inheriting from `IWorkingImageHardware` and `WorkingImageData`. Specific CPU implementations (like `WorkingImageCPU_Halide`) inherit from this class.
* **`IWorkingImageGPU`**: Abstract interface extending `IWorkingImageHardware` and `WorkingImageData`. Specific GPU implementations (like `WorkingImageGPU_Halide`) inherit from this interface.
### Concrete Implementations
* **`WorkingImageCPU_Halide`**: Concrete implementation inheriting from `WorkingImageCPU` and `WorkingImageHalide`. Combines CPU-specific logic, raw data management (`WorkingImageData`), and Halide buffer logic (`WorkingImageHalide`).
* **`WorkingImageGPU_Halide`**: Concrete implementation inheriting from `IWorkingImageGPU` and `WorkingImageHalide`. Combines GPU-specific logic (device transfers), raw data management (`WorkingImageData`), and Halide buffer logic (`WorkingImageHalide`).
### `CaptureMoment::Core::ImageProcessing::WorkingImageFactory` (Factory & Registry Pattern)
This factory encapsulates the logic for creating the appropriate `IWorkingImageHardware` implementation based on the configured backend.
* **Responsibility:** Centralize creation logic to avoid duplication in `StateImageManager` and `PhotoTask`.
* **Usage:** Both `StateImageManager` and `PhotoTask` use this factory to create working images, ensuring consistent initialization.
* **Pattern Explanation (Factory):** The `WorkingImageFactory` encapsulates the creation logic, hiding the complexity of instantiating different concrete types (`WorkingImageCPU_Halide`, `WorkingImageGPU_Halide`). The Registry pattern (using an `unordered_map` of creators) allows new backends (e.g., `WorkingImageCUDA`, `WorkingImageVulkan`) to be registered at startup without modifying the factory's core code, enhancing extensibility.
### Backend Selection
* **`CaptureMoment::Core::Config::AppConfig`:** Singleton that stores the selected processing backend (`MemoryType::CPU_RAM` or `MemoryType::GPU_MEMORY`).
* **`BenchmarkingBackendDecider`:** Performs runtime benchmarking to determine the optimal backend and stores the result in `AppConfig`.
* **Initialization:** The benchmark runs once at application startup in `main()`, and the result is used throughout the application lifecycle.
* [**See more**](core/IMAGE_PROCESSING.md).
---
## 4. WorkingImage Lifecycle Management
To manage the creation, reuse, and state of `IWorkingImageHardware` instances efficiently, a dedicated context class is introduced.
### `CaptureMoment::Core::ImageProcessing::WorkingImageContext` (Context Pattern)
* **Responsibility:** Manages the lifecycle of a single `IWorkingImageHardware` instance. It encapsulates the creation, reuse, and export of a `WorkingImage`.
* **Key Methods:**
* `prepare(std::unique_ptr<Common::ImageRegion>&& original_tile)`: Creates or reuses the internal `WorkingImage` instance based on the provided tile.
* `update(const Common::ImageRegion& original_tile)`: Updates the existing internal `WorkingImage` with new data.
* `getWorkingImage()`: Returns a shared pointer to the managed `IWorkingImageHardware`.
* `getWorkingImageAsRegion()`: Exports the current internal `WorkingImage` data to a CPU-based `ImageRegion`.
* `isReady()`: Checks if a `WorkingImage` is available and valid.
* `release()`: Releases the managed `WorkingImage`.
* **Pattern Explanation (Context):** `WorkingImageContext` acts as a context or wrapper around the specific `IWorkingImageHardware` instance (e.g., `WorkingImageCPU_Halide`, `WorkingImageGPU_Halide`). It centralizes the logic for its management (creation, update, validation, export), isolating the client code (like `StateImageManager`) from the direct instantiation and state management details of the various `WorkingImage` implementations.
* **Interaction with `OperationPipelineExecutor`:** The `Worker` (e.g., `HalideOperationWorker`) retrieves the `WorkingImage` instance from `WorkingImageContext` via `getWorkingImage()` and passes it to `PipelineHalideOperationManager::execute`. `OperationPipelineExecutor::execute` then receives this object via the `IWorkingImageHardware&` parameter. It subsequently calls methods like `isValid()`, `getSize()`, `getChannels()` on this object, which resolve to the specific implementations in the concrete `WorkingImage` class (e.g., `WorkingImageCPU_Halide`), interacting with the underlying `WorkingImageData` and `WorkingImageHalide` bases as described in Section 3. Finally, `OperationPipelineExecutor` accesses the `Halide::Buffer<float>` via a cast to `WorkingImageHalide&` to execute the pipeline.
---
## 5. Task-Based Processing Architecture: Abstraction and Orchestration
To manage the processing workflow effectively, especially in a potentially concurrent or sequential context, we introduce a layer of abstraction using interfaces and a central orchestrator.
### `CaptureMoment::Core::Domain::IProcessingTask` & `CaptureMoment::Core::Engine::PhotoTask` (Command Pattern / Task Abstraction)
The **`IProcessingTask`** interface defines a unit of work encapsulating the processing of an image region with a specific sequence of operations.
* **Responsibility:** Encapsulate the data (input tile, operations, factory), the execution logic (`execute`), and provide access to the result (`result`) and progress (`progress`).
* **Decoupling:** The interface abstracts the *how* of processing, allowing for different implementations (e.g., CPU-based, GPU-based in the future) or different types of tasks (e.g., loading, saving).
* **Implementation (`PhotoTask`):** A concrete implementation that applies a sequence of operations defined by `OperationDescriptors` to an `IWorkingImageHardware` using the static `OperationPipeline::applyOperations` method. It creates the working image using `WorkingImageFactory` based on the configured backend.
* **Pattern Explanation (Command):** The `IProcessingTask` interface represents a command (a unit of work). `PhotoTask` is a concrete command object that encapsulates all the necessary data (`OperationDescriptors`, `IWorkingImageHardware`) and the logic to execute the processing. This pattern allows for queuing, scheduling, and potentially undoing/redoing tasks, making the processing pipeline more flexible and manageable.
### `CaptureMoment::Core::Domain::IProcessingBackend` & `CaptureMoment::Core::Engine::PhotoEngine` (Orchestrator / Facade)
The **`IProcessingBackend`** interface defines the contract for creating and submitting **`IProcessingTasks`**.
* **Responsibility:** Orchestrate the overall processing flow. This includes managing the state of the loaded image (`SourceManager`), maintaining the list of active operations, creating **`IProcessingTasks`** via `createTask`, and handling their execution via `submit`. It also manages committing the final result back to the source via `commitResult`.
* **Implementation (`PhotoEngine`):** The main concrete orchestrator. It uses the `SourceManager` to load images and provide initial data (`tiles`). It executes tasks (synchronously in v1 via provides methods like `getWorkingImageAsRegion()` to export the hardware buffer to CPU for display purposes.
* **Pattern Explanation (Facade):** `PhotoEngine` acts as a facade, providing a simplified interface to the complex subsystem of `SourceManager`, `StateImageManager`, `OperationPipeline`, and task execution. It hides the intricate details of how these components interact, making it easier for the UI layer to initiate processing.
* **Benefit:** This centralizes the high-level logic (when to process, what operations are active, how to handle results, managing the working state) away pipeline execution and resource management.
---
## 6. Low-Level Pipeline Execution: Statelessness and Utility
The core logic for applying a sequence of operations to a data unit is encapsulated in a stateless utility located within the `operation` directory.
### `CaptureMoment::Core::Operations::OperationPipeline` (Stateless Utility)
The `OperationPipeline` class is refactored into a stateless class containing only a static method (`applyOperations`).
* **Responsibility:** Execute a sequence of operations on a given `IWorkingImageHardware`. It iterates through the `OperationDescriptor`s, uses the `OperationFactory` to create instances of the required operations, and executes them sequentially on the provided working image.
* **Decoupling:** It is independent of `SourceManager`, `PhotoEngine`, or any task management. It only knows about `IWorkingImageHardware`, `OperationDescriptor`, and `OperationFactory`.
* **Location:** This class resides in the `operations` folder, centralizing the low-level processing logic within the operation domain.
* **Benefit:** High reusability and testability. It's a pure function-like component that performs the core processing step.
* **Pattern Explanation (Utility/Stateless):** Being stateless means the class doesn't hold any instance-specific data. Its methods are pure functions of their inputs. This makes it highly reusable, thread-safe (as it doesn't modify shared state), and easy to test in isolation.
---
## 7. Operation Management: The Factory Pattern
This pattern remains crucial for creating operation instances within the processing pipeline.
### Components
* **`CaptureMoment::Core::Operations::IOperation` (formerly `IOperation`):** The base interface for all operations (e.g., `OperationBrightness`, `OperationContrast`). It defines a single `execute(IWorkingImageHardware& working_image, const OperationDescriptor& descriptor)` method.
* **Benefit:** Polymorphism. The `OperationPipeline` can treat every operation the same way, regardless of whether it performs a simple brightness adjustment or a complex convolution.
* **`CaptureMoment::Core::Operations::IOperationFusionLogic`:** Interface for providing the Halide fusion logic of an operation. Defines `appendToFusedPipeline` method for combining operations into a single pipeline.
* **Benefit:** Enables pipeline fusion optimization by allowing operations to contribute their Halide logic to a combined computational graph.
* **`CaptureMoment::Core::Operations::OperationFactory` (formerly `OperationFactory`):** This component is responsible for knowing how to construct concrete implementations of `IOperation` based on an `OperationType` defined in the `OperationDescriptor`.
* **Benefit:** Adding a new operation (e.g., `OperationSaturation`) only requires defining the new class and registering it in the factory setup, without modifying the `OperationPipeline`'s core logic. This promotes high maintainability and scalability.
* **Pattern Explanation (Factory):** The `OperationFactory` centralizes the creation of `IOperation` subclasses. It maps an `OperationType` to a specific creation routine. This removes the need for `OperationPipeline` to have `if/else` or `switch` statements for every possible operation type, making it easy to add new operations without touching existing code.
* [🟦 **SEE OPERATIONS.md**](core/OPERATIONS.md).
---
## 8. Pipeline Fusion Architecture
The architecture now includes advanced pipeline fusion capabilities for optimal performance.
### `CaptureMoment::Core::Pipeline::IPipelineExecutor` & `CaptureMoment::Core::Pipeline::OperationPipelineExecutor` (Strategy Pattern)
* **`IPipelineExecutor`**: Abstract interface for executing a pre-built pipeline on an image.
* **`OperationPipelineExecutor`**: Concrete implementation that executes fused adjustment operation pipelines using Halide's computational graph optimization.
* **Pattern Explanation (Strategy):** `IPipelineExecutor` defines the algorithm for executing a pipeline. `OperationPipelineExecutor` provides a specific implementation using Halide fusion. Other strategies (e.g., `FallbackPipelineExecutor` for sequential execution) could implement the same interface, allowing the system to switch between them dynamically based on requirements or capabilities.
### `CaptureMoment::Core::Pipeline::OperationPipelineBuilder` (Builder Pattern)
* **Responsibility:** Builds and compiles the fused Halide pipeline for the stored operations.
* **Optimization:** Creates combined computational passes that eliminate intermediate buffer copies between operations.
* **Integration:** Works with `IOperationFusionLogic` implementations to chain operations into a single pipeline.
* **Pattern Explanation (Builder):** The `OperationPipelineBuilder` separates the construction of the complex `OperationPipelineExecutor` object (specifically its internal Halide graph and compiled pipeline) from its representation. This allows the same construction process to create different representations (e.g., different compiled pipelines based on the list of operations).
### Operation Fusion Integration
* **`IOperationFusionLogic`**: Operations implement this interface to provide their fusion logic via `appendToFusedPipeline`.
* **Sequential vs Fused**: Operations maintain both `execute` (for sequential processing) and `appendToFusedPipeline` (for pipeline fusion) methods.
* **[[maybe_unused]]**: Sequential `execute` methods are marked as unused when primarily using fused execution.
### Dynamic Input Binding Correction
* **Problem:** An earlier version of `OperationPipelineExecutor` attempted to statically compile the entire Halide pipeline, including the input node, leading to incorrect execution where the input buffer was not properly linked during runtime.
* **Solution:** The `OperationPipelineExecutor` now caches the *logic for chaining operations* (the fused computational graph excluding the input node) in a `std::function` (`m_operation_chain`). During execution (`executeOnHalideBuffer`), the input buffer is dynamically bound to create the final `Halide::Func`, which is then compiled and scheduled just-in-time before execution. This ensures the correct input data is processed by the fused pipeline while preserving the performance benefits of operation fusion.
---
## 9. Hardware-Agnostic Pipeline Execution
The fused pipeline system works seamlessly across different hardware backends.
### `CaptureMoment::Core::ImageProcessing::WorkingImageHalide` (Base Class)
* **Shared Infrastructure**: Base class providing common Halide buffer functionality for both CPU and GPU implementations.
* **Memory Management**: Uses `std::unique_ptr<float[]>` (allocated via `std::make_unique_for_overwrite`) as backing store for Halide buffers, enabling in-place modifications without unnecessary copies and avoiding zero-initialization overhead during allocation.
* **Direct Buffer Access**: Provides `getHalideBuffer()` method for direct pipeline execution.
* **Interaction with Pipeline Executor**: The `OperationPipelineExecutor` interacts with `WorkingImageHalide` by calling its `getHalideBuffer()` `Halide::Buffer<float>` and then executing the pipeline on it via `executeOnHalideBuffer`.
* **Initialization & Dimension Access**: `WorkingImageHalide` now provides methods like `initializeHalide(std::span<float>, ...)` and dimension getters (`getSizeByHalide`, `getChannelsByHalide`) that are used by its derived classes (`WorkingImageCPU_Halide`, `WorkingImageGPU_Halide`) to initialize the Halide buffer view and query its properties. These getters return `Common::ImageDim`, `Common::ImageChan`, `Common::ImageSize`.
### `CaptureMoment::Core::ImageProcessing::WorkingImageData` (Base Class)
* **Raw Data Storage**: Provides the underlying `std::unique_ptr<float[]>` (`m_data`) and metadata (`m_width`, `m_height`, `m_channels`, `m_valid`) for CPU and GPU implementations. Uses `Common::ImageDim`, `Common::ImageChan`, `Common::ImageSize` for its members.
* **Initialization Logic**: Contains the `initializeData` method, which handles the allocation and copying of pixel data from an `ImageRegion`, and sets the metadata.
* **State Management**: The `m_valid` flag is managed here and used by derived classes to determine the overall validity state.
### `CaptureMoment::Core::Managers::StateImageManager` (Centralized Management)
* **Ownership of SourceManager:** Now owns `m_source_manager` exclusively, decoupling `PhotoEngine` from direct I/O concerns.
* **Delegated Responsibilities:** Acts as a coordinator between `SourceManager`, `PipelineContext`, and `WorkerContext`, preparing data and delegating execution.
* **Enhanced Interface:** Provides methods like `loadImage`, `commitWorkingImageToSource`, and getters for source image properties (`width`, `height`, `channels`).
* **Operation Coalescing:** Implements a coalescing strategy for incoming `applyOperations` requests. If an operation is already in progress (`isUpdatePending` is true), subsequent requests overwrite any previously pending request, optimizing for the most recent state during rapid UI interactions (e.g., dragging a slider).
* **WorkingImage Context:** Now owns `m_working_image_context` (`std::unique_ptr<WorkingImageContext>`) to manage the lifecycle of the active working image.
---
## 10. Pipeline Management and Execution Strategies
Recent refactoring introduced a more structured approach to managing pipeline execution strategies.
### `CaptureMoment::Core::Pipeline::PipelineBuilder* **Responsibility:** Central registry for creating `IPipelineExecutor` instances based on `PipelineType` (e.g., HalideOperation).
* **Global Instance:** The `PipelineBuilder` registry is now initialized once globally via `PipelineRegistry::registerAll()` during application startup (often triggered by `PipelineContext` construction).
* **Pattern Explanation (Registry):** This uses a static map (managed by `PipelineRegistry`) to associate a `PipelineType` with a creator function. This allows new pipeline types (e.g., AI-based, OpenCV-based) to be registered at startup without modifying existing managers like `StateImageManager`. It decouples the high-level orchestrator from the specific executor creation logic. Executors are created via the **static method** `PipelineBuilder::build()`.
### `CaptureMoment::Core::Pipeline::PipelineRegistry`
* **Responsibility:** Static helper to populate the global `PipelineBuilder` registry at startup.
### `CaptureMoment::Core::Pipeline::PipelineContext`
* **Responsibility:** Central container holding instances of `IPipelineManager`. It triggers the global registration of pipeline types via `PipelineRegistry::registerAll()` upon construction.
* **Pattern Explanation (Container/Service Locator):** `PipelineContext` acts as a service locator for the pipeline infrastructure, holding the active `IPipelineManager` instances. It does **not** hold a `PipelineBuilder` instance itself, relying instead on the global registry.
### `CaptureMoment::Core::Strategies::IPipelineManager` (Abstract Interface)
* **Responsibility:** High-level interface for managing a specific category of image processing (e.g., Halide adjustments).
* **Contract:** Defines `init(operations, factory)` and `execute(working_image)` methods.
* **Pattern Explanation (Strategy):** `IPipelineManager` defines the strategy for managing a specific type of pipeline execution (e.g., Halide adjustments). `PipelineHalideOperationManager` implements this strategy, encapsulating the lifecycle and configuration of the `OperationPipelineExecutor`.
### `CaptureMoment::Core::Strategies::PipelineHalideOperationManager` (Concrete Strategy)
* **Responsibility:** Concrete implementation managing Halide-based adjustment operations.
* **Encapsulation:** Handles the lifecycle and initialization of `OperationPipelineExecutor` based on the list of operations.
* **Thread Safety:** Includes mutex protection for concurrent access during initialization and execution.
* **Performance Optimization:** Implements `updateRuntimeParams` and tracks `m_last_operations` to detect structural changes vs. value-only updates, enabling fast parameter adjustments without recompilation.
* **Operation Factory Location:** Now owns the `OperationFactory` instance, centralizing operation creation logic.
* **Executor Creation:** Instantiates its required `OperationPipelineExecutor` by calling the **static** `PipelineBuilder::build()` method.
---
## 11. Asynchronous Processing Workers
A new layer has been introduced to handle specific processing tasks asynchronously, further decoupling execution logic.
### `CaptureMoment::Core::Workers::IWorkerRequest` (Abstract Interface)
* **Responsibility:** Defines the contract for executing a specific task asynchronously.
* **Contract:** Defines `execute(context, image)` returning a `std::future<bool>`.
* **Pattern Explanation (Strategy):** `IWorkerRequest` defines the strategy for asynchronous execution. Concrete workers (e.g., `HalideOperationWorker`) implement this strategy for specific tasks.
### `CaptureMoment::Core::Workers::WorkerType` (Enumeration)
* **Responsibility:** Identifies specific worker implementations for the registry.
### `CaptureMoment::Core::Workers::WorkerBuilder` (Registry Pattern)
* **Responsibility:** Central registry for creating `IWorkerRequest` instances based on `WorkerType`.
* **Pattern Explanation (Registry):** Similar to `PipelineBuilder`, uses a map to create worker instances dynamically.
### `CaptureMoment::Core::Workers::WorkerRegistry`
* **Responsibility:** Static helper to populate the `WorkerBuilder` registry.
### `CaptureMoment::Core::Workers::WorkerContext`
* **Responsibility:** Central container holding the `WorkerBuilder` and instances of `IWorkerRequest`.
* **Pattern Explanation (Container/Service Locator):** Acts as a service locator for worker infrastructure.
### `CaptureMoment::Core::Workers::HalideOperationWorker` (Concrete Worker)
* **Responsibility:** Concrete implementation for executing Halide-based adjustments asynchronously.
* **Pattern Explanation (Strategy):** Implements `IWorkerRequest` to execute the logic managed by `PipelineHalideOperationManager`.
* **WorkingImage Context Interaction:** Retrieves the `IWorkingImageHardware` instance to process via `WorkingImageContext::getWorkingImage()`.
---
## 12. Synchronized Operation Execution
A critical improvement has been made to ensure UI responsiveness and visual consistency### `CaptureMoment::Core::Engine::PhotoEngine` (Updated Contract)
* **Method Change:** The `applyOperations` method now returns a `std::future<bool>` instead of `void`.
* **Responsibility:** The caller (e.g., `ImageControllerBase`) is now responsible for calling `.get()` on the returned future to wait for the operation to complete. This ensures the internal working image state and the update flag in `StateImageManager` are properly synchronized before proceeding.
* **Benefit:** Prevents "Update already in progress" scenarios from causing stale display updates, ensuring the displayed image reflects the most recently applied operations.
### `CaptureMoment::Core::UI::ImageControllerBase` (Updated Behavior)
* **Method Change:** `doApplyOperations` now calls `m_engine->applyOperations(...)` and waits for the returned future using `.get()`.
* **Responsibility:** Ensures that the display update (`DisplayManager`) only occurs *after* the core image processing (`StateImageManager`) has fully finished and its state is updated.
* **Benefit:** Guarantees visual consistency between the UI controls and the rendered image.
---
## 13. Serialization and Persistence: Interfaces and Strategies (Independent Layer)
The core library includes a flexible system for saving and loading the state of image operations using XMP metadata. This system is designed as an **independent layer**, separate from the core image processing engine (`PhotoEngine`), to maximize modularity and flexibility.
### Components
* **`CaptureMoment::Core::Serializer::IXmpProvider`:** An interface abstracting the low-level XMP packet read/write operations. This allows switching between different XMP libraries (e.g., Exiv2, Adobe XMP Toolkit) without changing dependent code.
* **Pattern Explanation (Strategy):** `IXmpProvider` defines the strategy for interacting with XMP data. Different providers (`Exiv2Provider`) implement this strategy using specific libraries.
* **Implementation (`Exiv2Provider`):** A concrete implementation using the Exiv2 library to interact with XMP packets within image files.
* **`CaptureMoment::Core::Serializer::IXmpPathStrategy`:** An interface defining how to determine the file path for the XMP data associated with a given image path. This allows for different storage strategies (sidecar files, AppData directory, configurable path).
* **Pattern Explanation (Strategy):** `IXmpPathStrategy` defines the strategy for locating XMP files. Different strategies (`SidecarXmpPathStrategy`, `AppDataXmpPathStrategy`) implement this differently.
* **Implementations (`SidecarXmpPathStrategy`, `AppDataXmpPathStrategy`, `ConfigurableXmpPathStrategy`):** Concrete implementations of the path strategy interface, each defining its own logic for mapping an image path to an XMP file path.
* **`CaptureMoment::Core::Serializer::IFileSerializerWriter` & `CaptureMoment::Core::Serializer::IFileSerializerReader`:** Interfaces for writing and reading operation data to/from a file format (currently XMP). They depend on `IXmpProvider` and `IXmpPathStrategy`.
* **Pattern Explanation (Interface Segregation):** Separate writer and reader interfaces allow for independent implementation and testing.
* **Implementations (`FileSerializerWriter`, `FileSerializerReader`):** Concrete implementations that use the injected provider and strategy to perform the actual serialization/deserialization of `OperationDescriptor` lists to/from XMP packets.
* **`CaptureMoment::Core::Serializer::FileSerializerManager`:** A high-level manager that orchestrates the `IFileSerializerWriter` and `IFileSerializerReader`. It provides a unified interface (`saveToFile`, `loadFromFile`) for **external UI/QML layers** to use, not directly managed by `PhotoEngine`.
* **Pattern Explanation (Facade):** `FileSerializerManager` provides a simplified interface to the complex serialization subsystem (`IXmpProvider`, `IXmpPathStrategy`, `IFileSerializerWriter/Reader`).
* **`CaptureMoment::Core::Serializer::OperationSerialization`:** A namespace containing utility functions (`serializeParameter`, `deserializeParameter`) for converting `std::any` parameter values within `OperationDescriptor` to/from string representations suitable for storage in XMP metadata, preserving type information. This module relies on **generic conversion utilities** from the `CaptureMoment::Core::utils` namespace.
* **Pattern Explanation (Namespace/Utilities):** A namespace groups related utility functions. **Note:** The use of `std::any` for operation parameters is planned to be replaced by `std::variant` for better type safety and performance.
* **`CaptureMoment::Core::Serializer::Exiv2Initializer`:** A utility class ensuring the Exiv2 library is initialized before any operations are performed.
### Benefits of Independence
* **Modularity:** `PhotoEngine` focuses purely on image processing orchestration. The serialization logic is completely separate.
* **Flexibility:** The serialization layer (`FileSerializerManager`) can be managed and invoked independently by the UI layer (e.g., via `UISerializerManager` in the Qt module) without requiring `PhotoEngine` to hold a reference to it.
* **Maintainability:** Changes to serialization mechanisms or strategies do not impact the core processing engine.
* **Clear Responsibility:** `PhotoEngine` handles image processing state and pipeline execution. A separate service handles persistence.
* [🟦 **SEE SERIALIZER.md**](core/SERIALIZER.md).
---
## 14. Utility Modules and Generic Conversion
Generic utility functions, such as string conversion, are centralized to promote reusability and reduce code duplication across the core library.
### `CaptureMoment::Core::utils::toString`
* **Purpose:** Provides a generic mechanism for converting primitive types (e.g., `int`, `float`, `double`, `bool`) and `std::string` to their string representation.
* **Implementation:** Utilizes C++20 Concepts (`ToStringablePrimitive`) to constrain the template and ensure type safety. The core logic relies on `std::to_string` for numeric types and specific logic for `bool` and `std::string`.
* **Location:** Implemented in `utils/to_string_utils.h`, placed directly in the `utils` folder without subdirectories for conversion or other purposes.
* **Usage:** Replaces legacy specific functions like `serializeFloat`, `serializeDouble`, etc., within the serialization module and other parts of the core requiring type-to-string conversion.
---
## 15. Namespace Organization
The codebase is structured using a clear namespace hierarchy to improve modularity and maintainability:
- **`CaptureMoment::Core::Common`**: Contains fundamental data structures like `ImageRegion` and `PixelFormat`.
- **`CaptureMoment::Core::Operations`**: Contains operation-related logic, including `IOperation`, `OperationDescriptor`, `OperationFactory`, `OperationPipeline`, and specific operation implementations (e.g., `OperationBrightness`).
- **`CaptureMoment::Core::Pipeline`**: Contains pipeline fusion logic, including `IPipelineExecutor`, `OperationPipelineExecutor`, `PipelineBuilder`, `PipelineContext`, and `PipelineType`.
- **`CaptureMoment::Core::Strategies`**: Contains high-level processing strategies, including `IPipelineManager` and `PipelineHalideOperationManager`.
- **`CaptureMoment::Core::Workers`**: Contains asynchronous processing workers, including `IWorkerRequest`, `WorkerContext`, `WorkerBuilder`, and concrete workers like `HalideOperationWorker`.
- **`CaptureMoment::Core::Managers`**: Contains managers responsible for resource handling, such as `ISourceManager` and `SourceManager`.
- **`CaptureMoment::Core::Domain`**: Contains domain-specific interfaces, such as `IProcessingTask` and `IProcessingBackend`.
- **`CaptureMoment::Core::Engine`**: Contains the core application logic orchestrators, such as `PhotoTask` and `PhotoEngine`.
- **`CaptureMoment::Core::Serializer`**: Contains serialization-related interfaces, implementations, and utilities (e.g., `IXmpProvider`, `FileSerializerWriter`, `OperationSerialization`).
- **`CaptureMoment::Core::ImageProcessing`**: Contains the hardware abstraction layer, including `IWorkingImageHardware`, concrete implementations, factories, deciders, and the shared `WorkingImageHalide` base class.
- **`CaptureMoment::Core::Config`**: Contains application-wide configuration (e.g., `AppConfig`).
- **`CaptureMoment::Core::utils`**: Contains generic utility functions, such as `toString`.
This organization clarifies the role of each component and prevents naming collisions.
---
## 16. Recent Architectural Improvements
### Pipeline Fusion Optimization
- **Fused Execution**: Operations now support both sequential (`execute`) and fused (`appendToFusedPipeline`) execution patterns.
- **Zero-Copy Processing**: `WorkingImageHalide` base class eliminates unnecessary data copying by sharing memory between `std::unique_ptr<float[]>` and `Halide::Buffer`.
- **Hardware Agnostic**: Same fusion logic works for both CPU and GPU backends through the unified interface.
- **Dynamic Binding Correction**: Fixed pipeline execution to correctly bind input buffers at runtime, resolving issues with static compilation.
### Simplified PhotoEngine Architecture
- **Reduced Coupling:** `PhotoEngine` constructor now has zero parameters, with internal managers handling their own dependencies. It no longer directly owns `SourceManager`.
- **Centralized Management:** `StateImageManager` now owns `m_source_manager`, `m_pipeline_builder`, and `m_operation_factory` for better encapsulation and clearer responsibilities.
- **Automatic Registration:** Operation factory registration happens internally within `StateImageManager`.
### Enhanced Performance
- **In-Place Processing:** Halide buffers operate directly on shared data vectors, eliminating redundant copies.
- **Optimized Scheduling:** Pipeline fusion creates single computational passes instead of multiple sequential operations.
- **Backend Selection:** Runtime benchmarking automatically determines optimal CPU/GPU usage.
- **Memory Allocation:** `WorkingImageHalide` uses `std::unique_ptr<float[]>` with `std::make_unique_for_overwrite` to avoid zero-initialization overhead during large buffer allocation.
### WorkingImage Refactoring
- **New Base Classes:** Introduced `WorkingImageData` (for raw data and metadata) and `WorkingImageHalide` (for shared Halide logic).
- **Hierarchical Structure:** Concrete implementations like `WorkingImageCPU_Halide` and `WorkingImageGPU_Halide` now inherit from specific base classes (`WorkingImageCPU`/`IWorkingImageGPU`) which themselves inherit from `IWorkingImageHardware` and `WorkingImageData`. They also inherit from `WorkingImageHalide`.
- **Unified Buffer Initialization:** `WorkingImageHalide::initializeHalide` now accepts a `std::span<float>` for safer and more flexible buffer view creation.
- **Delegated Accessors:** Concrete implementations (`WorkingImageCPU_Halide`, `WorkingImageGPU_Halide`) delegate dimension and channel queries to methods defined in `WorkingImageHalide` (e.g., `getSizeByHalide`).
- **Validity Check Refined:** The `isValid()` method in concrete implementations now combines state from `WorkingImageData` (`m_valid`) and `WorkingImageHalide` (`m_halide_buffer.defined()`).
- **`exportToCPUMove` Removed:** The destructive `exportToCPUMove` method was removed from the interface and implementations. A new `downsample` method was introduced on `IWorkingImageHardware` and implemented in the concrete classes for optimized WorkingImage Context Integration
- **`StateImageManager` Ownership:** `StateImageManager` now owns `m_working_image_context` to manage the lifecycle of the active working image.
- **Worker Interaction:** Workers (e.g., `HalideOperationWorker`) retrieve the `IWorkingImageHardware` instance via `WorkingImageContext::getWorkingImage()`.
- **Pipeline Executor Interaction:** `OperationPipelineExecutor` receives the `IWorkingImageHardware` object via the `execute` method parameter, which originates from `WorkingImageContext`. It then interacts with this object as described in Section 3 and Section 9.
### Pipeline Management Refactoring
- **Global Registry Pattern**: Introduced `PipelineBuilder` (global registry) and `PipelineRegistry` for flexible executor creation. `PipelineContext` triggers global registration.
- **Strategy Pattern**: Introduced `IPipelineManager` and `PipelineHalideOperationManager strategy control.
- **Pipeline Context**: Centralized infrastructure management via `PipelineContext` (holds managers, triggers global builder registration).
### Asynchronous Worker System
- **IWorkerRequest Interface**: Defined the contract for asynchronous processing tasks.
- **Worker Context**: Centralized container for worker infrastructure.
- **Registry Pattern**: Introduced `WorkerBuilder` and `WorkerRegistry` for flexible worker creation.
- **Coordination:** `StateImageManager` acts as a coordinator, delegating execution to workers via `WorkerContext`.
### Move Semantics Optimization
- **Efficient Data Transfer:** `applyOperations` in `StateImageManager` and `PhotoEngine` now uses move semantics (`std::move`) for operation vectors, reducing unnecessary copies.
### Operation Fusion Logic Update
- **Halide Parameters:** Operations now pass `Halide::Param<float>` to `appendToFusedPipeline` instead of the full `OperationDescriptor`, enabling efficient runtime parameter updates.
### Operation Descriptor Enhancement
- **Unique Identifier:** `OperationDescriptor` now includes a `uint64_t id` field, generated atomically using `OperationDescriptor::generateId()`. This provides a stable, unique key for each operation instance.
- **Cache Key:** The `OperationPipelineExecutor` now uses this `id` as the key for its` cache (replacing the previous use of `name`).
- **Structure Detection:** `PipelineHalideOperationManager` now uses the `id` for precise structural change detection in its `init` method, comparing `operations[i].id` with `m_last_operations[i].id`.
- **Improved Robustness:** This change makes the pipeline update logic more robust by relying on a truly unique and stable identifier instead of potentially changing names or types.
### Operation Manager Optimization
- **Runtime Parameter Updates:** `PipelineHalideOperationManager` implements `updateRuntimeParams` to update pipeline parameters quickly without recompilation if only values change.
- **Structural Change Detection:** Uses `m_last_operations` to detect structural changes (add/remove/modify type/enable) versus value-only changes.
- **Global Builder Usage:** `PipelineHalideOperationManager` retrieves its executor via the **static** `PipelineBuilder::build()` method.
### StateImageManager as Central Coordinator
- **Exclusive Source Management:** `StateImageManager` now owns and manages `SourceManager` internally, providing a unified interface for image loading (`loadImage`), committing results (`commitWorkingImageToSource`), and querying source properties (`getWidth`, `getHeight`, `getChannels`).
- **Simplified PhotoEngine:** `PhotoEngine` delegates image loading and metadata queries to `StateImageManager`.
### Operation Factory Relocation
- **Centralized Ownership:** The `OperationFactory` is now owned by `PipelineHalideOperationManager` instead of `StateImageManager`, aligning responsibility with the entity that uses it for pipeline construction.
### Synchronized Operation Execution
- **Future-based Contract:** `PhotoEngine::applyOperations` now returns a `std::future<bool>`.
- **Caller Waits:** `ImageControllerBase` waits for the future to complete before updating the display, ensuring visual consistency and preventing stale updates.
### Operation Coalescing in StateImageManager
- **Coalescing Strategy:** `StateImageManager::applyOperations` now implements a coalescing strategy. If an operation is already pending, new requests overwrite the previous pending request, optimizing for the most recent state during rapid UI interactions.
- **Promise Handling:** The `std::future` returned by `applyOperations` resolves only when the *final* operation in the potential chain (including any coalesced ones) completes.
### Uniform Image Types
- **Common Types:** `Common::ImageDim`, `Common::ImageCoord`, `Common::ImageChan`, `Common::ImageSize` for consistent representation of image properties across `ImageRegion`, `SourceManager`, `WorkingImage` implementations, and related classes.
- **Consistent APIs:** Methods like `getSize()`, `getChannels()`, `getTile()` now return these common types, improving type safety and code readability.
---
## READ MORE
* [**Operations**](core/OPERATIONS.md).
* [**Image Processing**](core/IMAGE_PROCESSING.md).
* [**Serializer**](core/SERIALIZER.md).
---
