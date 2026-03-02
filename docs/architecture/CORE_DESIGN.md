# Core Architecture Design Principles
The CaptureMoment core library is designed with modularity, high performance, and future extensibility in mind, which is essential for a modern, tile-based image processing engine. This document outlines the key architectural decisions, focusing on how we separate data from behavior and use established design patterns.

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
* **Pattern Explanation (Strategy):** The `ISourceManager` interface defines a family of algorithms (different I/O methods). `SourceManager` provides a concrete implementation. This allows switching between different I/O strategies (e.g., OIIO, native filesystem, network) without changing the code that depends on `ISourceManager`, promoting flexibility and maintainability.
---
## 3. Hardware-Agnostic Processing Architecture
The most significant recent evolution is the introduction of a hardware-agnostic processing layer that abstracts CPU and GPU execution behind a unified interface.
### `CaptureMoment::Core::ImageProcessing::IWorkingImageHardware` (Abstract Interface)
This interface represents an image used as a working buffer, abstracting its hardware location (CPU RAM or GPU memory).
* **Key Methods:**
* `exportToCPUCopy()`: Creates a CPU copy of the image data for display or saving.
* `updateFromCPU(const ImageRegion&)`: Updates the working buffer from CPU data.
* `isValid()`: Checks if the buffer is valid.
* `getSize()`, `getChannels()`, `getDataSize()`: Query buffer dimensions.
* **Pattern Explanation (Interface/Abstraction):** The interface `IWorkingImageHardware` defines the contract for interacting with an image buffer, regardless of its physical location. Concrete implementations (`WorkingImageCPU_Halide`, `WorkingImageGPU_Halide`) provide the specific logic for CPU or GPU memory. This allows the rest of the application (like `OperationPipelineExecutor`) to work with the abstract interface, promoting hardware independence and easier testing.
### Concrete Implementations
* **`WorkingImageCPU_Halide`:** Uses Halide buffers for CPU processing.
* **`WorkingImageGPU_Halide`:** Uses Halide buffers with GPU scheduling for GPU processing.
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
## 4. Task-Based Processing Architecture: Abstraction and Orchestration
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
* **Implementation (`PhotoEngine`):** The main concrete orchestrator. It uses the `SourceManager` to load images and provide initial data (`tiles`). It executes tasks (synchronously in v1 via `submit`) and provides methods like `getWorkingImageAsRegion()` to export the hardware buffer to CPU for display purposes.
* **Pattern Explanation (Facade):** `PhotoEngine` acts as a facade, providing a simplified interface to the complex subsystem of `SourceManager`, `StateImageManager`, `OperationPipeline`, and task execution. It hides the intricate details of how these components interact, making it easier for the UI layer to initiate processing.
* **Benefit:** This centralizes the high-level logic (when to process, what operations are active, how to handle results, managing the working state) away from the low-level pipeline execution and resource management.
---
## 5. Low-Level Pipeline Execution: Statelessness and Utility
The core logic for applying a sequence of operations to a data unit is encapsulated in a stateless utility located within the `operation` directory.
### `CaptureMoment::Core::Operations::OperationPipeline` (Stateless Utility)
The `OperationPipeline` class is refactored into a stateless class containing only a static method (`applyOperations`).
* **Responsibility:** Execute a sequence of operations on a given `IWorkingImageHardware`. It iterates through the `OperationDescriptor`s, uses the `OperationFactory` to create instances of the required operations, and executes them sequentially on the provided working image.
* **Decoupling:** It is independent of `SourceManager`, `PhotoEngine`, or any task management. It only knows about `IWorkingImageHardware`, `OperationDescriptor`, and `OperationFactory`.
* **Location:** This class resides in the `operations` folder, centralizing the low-level processing logic within the operation domain.
* **Benefit:** High reusability and testability. It's a pure function-like component that performs the core processing step.
* **Pattern Explanation (Utility/Stateless):** Being stateless means the class doesn't hold any instance-specific data. Its methods are pure functions of their inputs. This makes it highly reusable, thread-safe (as it doesn't modify shared state), and easy to test in isolation.
---
## 6. Operation Management: The Factory Pattern
This pattern remains crucial for creating operation instances within the processing pipeline.
### Components
* **`CaptureMoment::Core::Operations::IOperation` (formerly `IOperation`):** The base interface for all operations (e.g., `OperationBrightness`, `OperationContrast`). It defines a single `execute(IWorkingImageHardware& working_image, const OperationDescriptor& descriptor)` method.
* **Benefit:** Polymorphism. The `OperationPipeline` can treat every operation the same way, regardless of whether it performs a simple brightness adjustment or a complex convolution.
* **`CaptureMoment::Core::Operations::IOperationFusionLogic`:** Interface for providing the Halide fusion logic of an operation. Defines `appendToFusedPipeline` method for combining operations into a single pipeline.
* **Benefit:** Enables pipeline fusion optimization by allowing operations to contribute their Halide logic to a combined computational graph.
* **`CaptureMoment::Core::Operations::OperationFactory` (formerly `OperationFactory`):** This component is responsible for knowing how to construct concrete implementations of `IOperation` based on an `OperationType` defined in the `OperationDescriptor`.
* **Benefit:** Adding a new operation (e.g., `OperationSaturation`) only requires defining the new class and registering it in the factory setup, without modifying the `OperationPipeline`'s core logic. This promotes high maintainability and scalability.
* **Pattern Explanation (Factory):** The `OperationFactory` centralizes the creation of `IOperation` subclasses. It maps an `OperationType` to a specific creation routine. This removes the need for `OperationPipeline` to have `if/else` or `switch` statements for every possible operation type, making it easy to add new operations without touching existing code.
* [**See more**](core/OPERATIONS.md).
---
## 7. Pipeline Fusion Architecture
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
## 8. Hardware-Agnostic Pipeline Execution
The fused pipeline system works seamlessly across different hardware backends.
### `CaptureMoment::Core::ImageProcessing::WorkingImageHalide` (Base Class)
* **Shared Infrastructure**: Base class providing common Halide buffer functionality for both CPU and GPU implementations.
* **Memory Management**: Uses `std::unique_ptr<float[]>` (allocated via `std::make_unique_for_overwrite`) as backing store for Halide buffers, enabling in-place modifications without unnecessary copies and avoiding zero-initialization overhead during allocation.
* **Direct Buffer Access**: Provides `getHalideBuffer()` method for direct pipeline execution.
### `CaptureMoment::Core::Managers::StateImageManager` (Centralized Management)
* **Ownership of SourceManager:** Now owns `m_source_manager` exclusively, decoupling `PhotoEngine` from direct I/O concerns.
* **Delegated Responsibilities:** Acts as a coordinator between `SourceManager`, `PipelineContext`, and `WorkerContext`, preparing data and delegating execution.
* **Enhanced Interface:** Provides methods like `loadImage`, `commitWorkingImageToSource`, and getters for source image properties (`width`, `height`, `channels`).
---
## 9. Pipeline Management and Execution Strategies
Recent refactoring introduced a more structured approach to managing pipeline execution strategies.
### `CaptureMoment::Core::Pipeline::PipelineBuilder` (Registry Pattern - Global)
* **Responsibility:** Central registry for creating `IPipelineExecutor` instances based on `PipelineType` (e.g., HalideOperation).
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
## 10. Asynchronous Processing Workers
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
---
## 11. Synchronized Operation Execution
A critical improvement has been made to ensure UI responsiveness and visual consistency during interactive adjustments.
### `CaptureMoment::Core::Engine::PhotoEngine` (Updated Contract)
* **Method Change:** The `applyOperations` method now returns a `std::future<bool>` instead of `void`.
* **Responsibility:** The caller (e.g., `ImageControllerBase`) is now responsible for calling `.get()` on the returned future to wait for the operation to complete. This ensures the internal working image state and the update flag in `StateImageManager` are properly synchronized before proceeding.
* **Benefit:** Prevents "Update already in progress" scenarios from causing stale display updates, ensuring the displayed image reflects the most recently applied operations.
### `CaptureMoment::Core::UI::ImageControllerBase` (Updated Behavior)
* **Method Change:** `doApplyOperations` now calls `m_engine->applyOperations(...)` and waits for the returned future using `.get()`.
* **Responsibility:** Ensures that the display update (`DisplayManager`) only occurs *after* the core image processing (`StateImageManager`) has fully finished and its state is updated.
* **Benefit:** Guarantees visual consistency between the UI controls and the rendered image.
---
## 12. Serialization and Persistence: Interfaces and Strategies (Independent Layer)
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
## 13. Utility Modules and Generic Conversion
Generic utility functions, such as string conversion, are centralized to promote reusability and reduce code duplication across the core library.
### `CaptureMoment::Core::utils::toString`
* **Purpose:** Provides a generic mechanism for converting primitive types (e.g., `int`, `float`, `double`, `bool`) and `std::string` to their string representation.
* **Implementation:** Utilizes C++20 Concepts (`ToStringablePrimitive`) to constrain the template and ensure type safety. The core logic relies on `std::to_string` for numeric types and specific logic for `bool` and `std::string`.
* **Location:** Implemented in `utils/to_string_utils.h`, placed directly in the `utils` folder without subdirectories for conversion or other purposes.
* **Usage:** Replaces legacy specific functions like `serializeFloat`, `serializeDouble`, etc., within the serialization module and other parts of the core requiring type-to-string conversion.
---
## 14. Namespace Organization
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
## 15. Recent Architectural Improvements
### Pipeline Fusion Optimization
- **Fused Execution**: Operations now support both sequential (`execute`) and fused (`appendToFusedPipeline`) execution patterns.
- **Zero-Copy Processing**: `WorkingImageHalide` base class eliminates unnecessary data copying by sharing memory between `std::unique_ptr<float[]>` and `Halide::Buffer`.
- **Hardware Agnostic**: Same fusion logic works for both CPU and GPU backends through the unified interface.
- **Dynamic Binding Correction**: Fixed pipeline execution to correctly bind input buffers at runtime, resolving issues with static compilation.
### Simplified PhotoEngine Architecture
- **Reduced Coupling**: `PhotoEngine` constructor now has zero parameters, with internal managers handling their own dependencies. It no longer directly owns `SourceManager`.
- **Centralized Management**: `StateImageManager` now owns `m_source_manager`, `m_pipeline_builder`, and `m_operation_factory` for better encapsulation and clearer responsibilities.
- **Automatic Registration**: Operation factory registration happens internally within `StateImageManager`.
### Enhanced Performance
- **In-Place Processing**: Halide buffers operate directly on shared data vectors, eliminating redundant copies.
- **Optimized Scheduling**: Pipeline fusion creates single computational passes instead of multiple sequential operations.
- **Backend Selection**: Runtime benchmarking automatically determines optimal CPU/GPU usage.
- **Memory Allocation**: `WorkingImageHalide` uses `std::unique_ptr<float[]>` with `std::make_unique_for_overwrite` to avoid zero-initialization overhead during large buffer allocation.
### Pipeline Management Refactoring
- **Global Registry Pattern**: Introduced `PipelineBuilder` (global registry) and `PipelineRegistry` for flexible executor creation. `PipelineContext` triggers global registration.
- **Strategy Pattern**: Introduced `IPipelineManager` and `PipelineHalideOperationManager` for high-level strategy control.
- **Pipeline Context**: Centralized infrastructure management via `PipelineContext` (holds managers, triggers global builder registration).
### Asynchronous Worker System
- **IWorkerRequest Interface**: Defined the contract for asynchronous processing tasks.
- **Worker Context**: Centralized container for worker infrastructure.
- **Registry Pattern**: Introduced `WorkerBuilder` and `WorkerRegistry` for flexible worker creation.
- **Coordination**: `StateImageManager` acts as a coordinator, delegating execution to workers via `WorkerContext`.
### Move Semantics Optimization
- **Efficient Data Transfer**: `applyOperations` in `StateImageManager` and `PhotoEngine` now uses move semantics (`std::move`) for operation vectors, reducing unnecessary copies.
### Operation Fusion Logic Update
- **Halide Parameters**: Operations now pass `Halide::Param<float>` to `appendToFusedPipeline` instead of the full `OperationDescriptor`, enabling efficient runtime parameter updates.
### Operation Manager Optimization
- **Runtime Parameter Updates**: `PipelineHalideOperationManager` implements `updateRuntimeParams` to update pipeline parameters quickly without recompilation if only values change.
- **Structural Change Detection**: Uses `m_last_operations` to detect structural changes (add/remove/modify type/enable) versus value-only changes.
- **Global Builder Usage**: `PipelineHalideOperationManager` retrieves its executor via the **static** `PipelineBuilder::build()` method.
### StateImageManager as Central Coordinator
- **Exclusive Source Management**: `StateImageManager` now owns and manages `SourceManager` internally, providing a unified interface for image loading (`loadImage`), committing results (`commitWorkingImageToSource`), and querying source properties (`getWidth`, `getHeight`, `getChannels`).
- **Simplified PhotoEngine**: `PhotoEngine` delegates image loading and metadata queries to `StateImageManager`.
### Operation Factory Relocation
- **Centralized Ownership**: The `OperationFactory` is now owned by `PipelineHalideOperationManager` instead of `StateImageManager`, aligning responsibility with the entity that uses it for pipeline construction.
### Synchronized Operation Execution
- **Future-based Contract**: `PhotoEngine::applyOperations` now returns a `std::future<bool>`.
- **Caller Waits**: `ImageControllerBase` waits for the future to complete before updating the display, ensuring visual consistency and preventing stale updates.
---
## READ MORE
* [**Operations**](core/OPERATIONS.md).
* [**Image Processing**](core/IMAGE_PROCESSING.md).
* [**Serializer**](core/SERIALIZER.md).
---
