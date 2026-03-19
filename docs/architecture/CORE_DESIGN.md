# Core Architecture Design Principles

The CaptureMoment core library is designed with modularity, high performance, and future extensibility in mind, which is essential for a modern, tile-based image processing engine. This document outlines the key architectural decisions, focusing on how we separate data from behavior and use established design patterns.

---

## 1. Data-Centric Design: Plain Old Data (POD) / Value Types

A fundamental principle of this architecture is the segregation of data storage from processing logic. Core data structures are kept simple and easy to handle:

* **`CaptureMoment::Core::Common::ImageRegion`**: A Plain Old Data (POD) struct representing a rectangular region of an image. It holds raw pixel data (`std::vector<float>`), dimensions (`m_width`, `m_height`), channel count (`m_channels`), and coordinates (`m_x`, `m_y`). Designed as a value type for efficient copying/moving by value (e.g., returning from a SourceManager), although deep copies of `m_data` are expensive. Prefer passing by `std::span<float>` in algorithms that only read data. Includes `isValid()` for overflow-safe integrity checks.

* **`CaptureMoment::Core::Operations::OperationDescriptor`**: A POD struct holding parameters for a single operation instance (e.g., `OperationType`, `OperationParams`, `uint64_t id`). It acts as a configuration snapshot passed to `IOperation` implementations. Designed as a value type for easy storage and transmission.

* **`CaptureMoment::Core::Common::OperationParams`**: A POD struct containing the specific parameters for an operation (e.g., `float brightness`, `float contrast`).

---

## 2. Manager and Engine Separation

The core functionality is split into specialized components: Managers handle resources and external concerns, while the Engine handles orchestration.

### `CaptureMoment::Core::Managers::ISourceManager` & `CaptureMoment::Core::Managers::SourceManager` (Strategy Pattern)

* **Problem:** Direct dependency on OpenImageIO (OIIO) throughout the core would make testing difficult and tightly couple the core logic to OIIO's API.
* **Solution:** Introduce `ISourceManager`. The concrete implementation `SourceManager` uses OIIO internally.
* **Impact:**
  * **Loose Coupling:** Higher-level components like `StateImageManager` depend only on the `ISourceManager` interface.
  * **Testability:** Mock implementations of `ISourceManager` can be used for unit tests.
  * **Abstraction:** The rest of the application is insulated from OIIO's specific complexities.

* **Implementation (`SourceManager`):** The concrete implementation uses industry-standard tools like OpenImageIO (OIIO) to handle diverse file formats and efficient caching via `OIIO::ImageBuf` and `OIIO::ImageCache rest of the application from OIIO's specific complexities.

---

## 3. Hardware Abstraction Layer (`IWorkingImageHardware`)

* **Problem:** Implementing image processing logic separately for CPU and multiple GPU APIs (CUDA, OpenCL, Metal, DX12, Vulkan) leads to massive code duplication and maintenance overhead.
* **Solution:** Introduce the `IWorkingImageHardware` interface. Concrete implementations (`WorkingImageCPU_Halide`, `WorkingImageGPU_Halide` and its descendants like `WorkingImageCUDA_Halide`, `WorkingImageVulkan_Halide`) handle platform-specific details.
* **Impact:**
  * **Unified Processing Logic:** Algorithms (like the fused Halide pipeline execution) operate on the `IWorkingImageHardware` interface, oblivious to the underlying hardware.
  * **Modularity:** Adding a new backend involves implementing the interface, without touching the core processing logic.
  * **Flexibility:** The system can dynamically choose the best backend (`AppConfig`, benchmarking) at runtime or initialization.

### `CaptureMoment::Core::ImageProcessing::WorkingImageHalide` (Base Class)

* **Shared Infrastructure**: Base class providing common Halide buffer functionality for both CPU and GPU implementations.
* **Memory Management**: Uses `std::unique_ptr<float[]>` (allocated via `std::make_unique_for_overwrite`) as backing store for Halide buffers, enabling in-place modifications without unnecessary copies and avoiding zero-initialization overhead during allocation.
* **Pattern Explanation (Template Method):** Defines common steps for buffer creation and management, allowing subclasses to customize specific parts (e.g., GPU buffer allocation/deallocation).

### WorkingImage Refactoring

* **New Base Classes:** Introduced `WorkingImageData` (for raw data and metadata) and `WorkingImageHalide` (for shared Halide logic).
* **Hierarchical Structure:** Concrete implementations like `WorkingImageCPU_Halide` and `WorkingImageGPU_Halide` now inherit from specific base classes (`WorkingImageCPU`/`IWorkingImageGPU`) which themselves inherit from `IWorkingImageHardware` and `WorkingImageData`. They also inherit from `WorkingImageHalide`.
* **Unified Buffer Initialization:** `WorkingImageHalide::initializeHalide` now accepts a `std::span<float>` for safer and more flexible buffer view creation.

### `CaptureMoment::Core::ImageProcessing::WorkingImageFactory` (Factory & Registry Pattern)

This factory encapsulates the logic for creating the appropriate `IWorkingImageHardware` implementation based on the configured backend.

* **Responsibility:** Centralize creation logic to avoid duplication in `StateImageManager` and `PhotoTask`.

---

## 4. Pipeline Fusion with Halide

* **Problem:** Applying multiple adjustments sequentially (e.g., brightness -> contrast -> saturation) involves multiple passes over the image buffer, leading to poor performance and potential rounding errors.
* **Solution:** Use Halide to fuse multiple operations into a single computational pipeline executed in one pass.
* **Impact:**
  * **Performance:** Dramatically reduces execution time by minimizing memory bandwidth usage and loop overhead.
  * **Quality:** Reduces cumulative floating-point errors by performing all calculations in a single pass.
  * **Hardware Agnostic:** The seamlessly across different hardware backends.

### Operation Fusion Logic Update

* **Halide Parameters:** Operations now pass `Halide::Param<float>` to `appendToFusedPipeline` instead of the full `OperationDescriptor`, enabling efficient runtime parameter updates.

### Dynamic Input Binding Correction

* **Problem:** An earlier version of `OperationPipelineExecutor` attempted to statically compile the entire Halide pipeline, including the input node, leading to incorrect execution where the input buffer was not properly linked during runtime.
* **Solution:** The pipeline execution was corrected to bind the input `IWorkingImageHardware` buffer dynamically at runtime before running the compiled pipeline.
* **Impact:**
  * **Correctness:** Ensures the pipeline operates on the intended image data.
  * **Flexibility:** Allows the same compiled pipeline to process different image instances.

### Pipeline Executor Interaction

* **`OperationPipelineExecutor` receives the `IWorkingImageHardware` object via the `execute` method parameter, which originates from `WorkingImageContext`. It then interacts with this object as described in Section 3 and Section 9.

---

## 5. Operation Management

* **Problem:** Hard-coding specific operations into the engine makes the system inflexible and difficult to extend.
* **Solution:** Use `OperationDescriptor` (POD) and `IOperation` (interface/factory pattern).
* **Impact:**
  * **Extensibility:** New operations can be added by implementing `IOperation` and registering them.
  * **Decoupling:** The `PhotoEngine` and `StateImageManager` don't need to know about specific operation types, only the `OperationDescriptor` and the `IOperation` interface.
  * **Persistence:** `OperationDescriptor` is easily serializable.

* **Why Value Type?** It acts as a configuration snapshot. The `OperationPipeline` uses this descriptor to instruct the `OperationFactory` what to create and what parameters to apply, keeping the engine itself free from hard-coded operation logic.

* **Sequential vs Fused**: Operations maintain both `execute` (for sequential processing) and `appendToFusedPipeline` (for pipeline fusion) methods.
* **[[maybe_unused]]**: Sequential `execute` methods are marked as unused when primarily using fused execution.

---

## 6. Asynchronous Workers

* **Problem:** Long-running tasks (like heavy image processing or I/O) can block the UI thread.
* **Solution:** Implement the Worker Pattern using `std::async` or similar for background execution.
* **Impact:**
  * **Responsiveness:** Keeps the UI thread free.
  * **Scalability:** Allows parallel execution of independent tasks.

* **Pattern Explanation (Worker/Active Object):** `IWorkerRequest` defines the work contract. `WorkerBuilder` creates specific worker instances (e.g., `HalideOperationWorker`). `WorkerContext` provides access to shared resources without tight coupling.

---

## 7. Error Handling and Logging

* **Problem:** Propagating errors through a complex, layered architecture without throwing exceptions everywhere.
* **Solution:** Use `std::expected<T, Error>` for functions that can fail, and `spdlog` for structured logging.
* **Impact:**
  * **Explicit Error Handling:** Forces callers to deal with potential failures.
  * **Debugging:** Comprehensive logs help diagnose issues.

---

## 8. Serialization System

* **Problem:** Saving and restoring the state of image adjustments in a standard, portable way.
* **Solution:** Use Exiv2 to embed operation descriptors into image file XMP metadata.
* **Impact:**
  * **Non-Destructive Editing:** Preserves original image data.
  * **Portability:** XMP is a widely supported standard.

* **Pattern Explanation (Facade):** `FileSerializerManager` provides a simplified interface to the complex serialization subsystem (`IXmpProvider`, `IXmpPathStrategy`, `IFileSerializerWriter/Reader`).

* **Usage:** ReFloat`, `serializeDouble`, etc., within the serialization module and other parts of the core requiring type-to-string conversion.

---

## 9. Memory Management

* **Problem:** Efficiently handling large image buffers without excessive copying or unnecessary initialization.
* **Solution:**
  * Use `std::span<float>` for non-owning views of data, minimizing copies when passing buffers to algorithms (like Halide).
  * Use `std::unique_ptr<float[]>` with `std::make_unique_for_overwrite` for owning allocations (e.g., in `WorkingImageHalide`), avoiding zero-initialization overhead for large buffers that are immediately filled.
* **Impact:**
  * **Performance:** Reduces allocation time and memory bandwidth usage.
  * **Safety:** RAII and smart pointers prevent leaks.

* **Memory Allocation:** `WorkingImageHalide` uses `std::unique_ptr<float[]>` with `std::make_unique_for_overwrite` to avoid zero-initialization overhead during large buffer allocation.

---

## 10. Configuration Management

* **Problem:** Managing settings like selected GPU backend, performance preferences, UI language, etc.
* **Solution:** Centralized `AppConfig` object holding these parameters, potentially loaded from a file.
* **Impact:**
  * **Maintainability:** Single source of truth for configuration.
  * **Flexibility:** Easy to modify behavior based on settings.

---

## 11. Performance Optimization Techniques

* **Zero-Copy Processing:** `WorkingImageHalide` base class eliminates unnecessary data copying by sharing memory between `std::unique_ptr<float[]>` and `Halide::Buffer`.
* **In-Place Processing:** Halide buffers operate directly on shared data vectors, eliminating redundant copies.
* **Optimized Scheduling:** Pipeline fusion creates single computational passes instead of multiple sequential operations.
* **Backend Selection:** Runtime benchmarking automatically determines optimal CPU/GPU usage.
* **Pipeline Fusion Optimization:** Fused Execution: Operations now support both sequential (`execute`) and fused (`appendToFusedPipeline`) execution patterns.

---

## 12. Data Flow and Immutability Considerations

* **Problem:** Managing state changes and ensuring data consistency in a pipeline with potential asynchronous operations.
* **Solution:**
  * `StateImageManager` acts as the single authority for the current "working image".
  * Operations modify the image *in-place* on the `IWorkingImageHardware` managed by `WorkingImageContext`.
  * Explicit synchronization points (e.g., waiting on futures from workers) ensure operations complete before dependent actions begin.
* **Impact:**
  * **Clarity:** Clear ownership and modification points.
  * **Consistency:** Ensures the displayed or saved image reflects the latest applied operations.

---

## 13. Testing Strategy

* **Unit Tests:** Individual components (Operations, Factories, Utils) are tested in isolation using mocks for dependencies.
* **Integration Tests:** Higher-level workflows (e.g., `StateImageManager::applyOperations`) are tested with real or near-real dependencies.
* **Performance Tests:** Benchmark critical paths (pipeline execution, loading times) to monitor regressions.

---

## 14. Future Extensibility

* **Plugin Architecture:** The Factory and Strategy patterns lay groundwork for potentially loading operations or backends dynamically as plugins.
* **New Backends:** The `IWorkingImageHardware` interface allows adding new hardware targets (e.g., future GPU APIs, specialized accelerators).
* **Advanced Pipelines:** The `IPipelineManager` strategy could be extended to manage different types of processing (e.g., AI inference, advanced filters).

---

## 15. Namespace Organization

The codebase is structured using a clear namespace hierarchy to improve modularity and maintainability:

- **`CaptureMoment::Core::Common`**: Contains fundamental data structures like `ImageRegion` and `PixelFormat`.
- **`CaptureMoment::Core::Engine`**: Contains the main orchestrator `PhotoEngine`.
- **`CaptureMoment::Core::Managers`**: Contains resource managers like `StateImageManager`, `SourceManager`.
- **`CaptureMoment::Core::Operations`**: Contains operation logic and descriptors.
- **`CaptureMoment::Core::Factories`**: Contains factory classes like `OperationFactory`.
- **`CaptureMoment::Core::ImageProcessing`**: Contains image buffer abstractions (`IWorkingImageHardware`) and processing helpers.
- **`CaptureMoment::Core::Pipeline`**: Contains pipeline fusion and execution logic (`IPipelineExecutor`, `PipelineContext`).
- **`CaptureMoment::Core::Strategies`**: Contains high-level processing strategies (`IPipelineManager`).
- **`CaptureMoment::Core::Workers`**: Contains asynchronous processing workers (`IWorkerRequest`).
- **`CaptureMoment::Core::Serialization`**: Contains serialization/deserialization logic (`FileSerializerManager`).

---

### StateImageManager as Central Coordinator

- **Exclusive Source Management:** `StateImageManager` now owns and manages `SourceManager` internally, providing a unified interface for image loading (`loadImage`), committing results (`commitWorkingImageToSource`), and querying source properties (`getWidth`, `getHeight`).
- **Simplified PhotoEngine:** `PhotoEngine` delegates image loading and metadata queries to `StateImageManager`.

---

### Simplified PhotoEngine Architecture

- **Reduced Coupling:** `PhotoEngine` constructor now has zero parameters, with internal managers handling their own dependencies. It no longer directly owns `SourceManager`.
- **Centralized Management:** `StateImageManager` now owns `m_source_manager`, `m_pipeline_builder`, and `m_operation_factory` for better encapsulation and clearer responsibilities.

---

### Pipeline Management Refactoring

- **Global Registry Pattern**: Introduced `PipelineBuilder` (global registry) and `PipelineRegistry` for flexible executor creation. `PipelineContext` triggers global registration.
- **Strategy Pattern**: Introduced `IPipelineManager` and `PipelineHalideOperationManager` to control the execution strategy of operation pipelines, allowing for different approaches (e.g., fused vs. sequential) based on requirements.

- **Global Builder Usage**: `PipelineContext` uses `PipelineBuilder::build()` to create the appropriate `IPipelineExecutor`.

---

### Operation Descriptor Enhancement & Pipeline Optimization

- **Unique Identifier:** `OperationDescriptor` now includes a `uint64_t id` field, generated atomically using `OperationDescriptor::generateId()`. This provides a stable, unique key for each operation instance.
- **Impact:**
  * **Cache Key:** The `OperationPipelineExecutor` now uses this `id` as the key for its cache (replacing the previous use of `name`).
  * **Structure Detection:** `PipelineHalideOperationManager` now uses the `id` for precise structural change detection in its `init` method, comparing `operations[i].id` with `m_last_operations[i].id`. This makes the pipeline update logic more robust by relying on a truly unique and stable identifier instead of potentially changing names or types.

---

### Synchronized Operation Execution

- **Problem:** Potential race conditions or stale display updates if UI tried to refresh before the core processing was complete.
- **Solution:** `PhotoEngine::applyOperations` now returns a `std::future<bool>`. The caller (e.g., `ImageControllerBase`) is responsible for waiting on this future (`.get()`) before proceeding with UI updates.
- **Impact:** Ensures visual consistency between the UI controls and the rendered image by guaranteeing the core image processing finishes before the display updates.

- **Method Change:** `ImageControllerBase::doApplyOperations` now calls `m_engine->applyOperations(...)` and waits for the returned future using `.get()`.
- **Responsibility:** Ensures that the display update (`DisplayManager`) only occurs after the core image processing (`StateImageManager`) has fully finished and its state is updated.
- **Benefit:** Guarantees visual consistency between the UI controls and the rendered image.

---

### Hardware-Agnostic Downsampling Abstraction

- **Problem:** Displaying high-resolution images efficiently required CPU-side downsampling, which could be a bottleneck. The UI layer (`DisplayManager`) previously handled this itself, tightly coupling display logic with image processing details.
- **Solution:** The `IWorkingImageHardware` interface (and its implementations like `WorkingImageCPU_Halide`, `WorkingImageGPU_Halide`) now exposes a dedicated `downsample(target_width, target_height)` method. This abstracts the *downsampling* operation behind the hardware abstraction layer.
- **Impact:**
  * **Core Integration:** The Core's `StateImageManager` and `Engine` now expose a new method `getDownsampledDisplayImage(width, height)`. `StateImageManager` delegates this call to `WorkingImageContext::getDownsampled` (renamed from `downsample` for clarity), which in turn invokes the `downsample` method on the active `IWorkingImageHardware` implementation.
  * **UI Decoupling:** The `PhotoEngine` serves as the primary entry point for the UI layer (`ImageControllerBase`) to request a display-ready, downsampled image. `ImageControllerBase` calls `m_engine->getDownsampledDisplayImage(...)` and receives the result (as `std::unique_ptr<Common::ImageRegion>`), which it then passes to the `DisplayManager`. This cleanly separates the Core's processing responsibilities from the UI's display management.
  * **Performance:** For GPU implementations (`WorkingImageGPU_Halide`), this means the downsampling operation can be performed *directly on the GPU* using a specialized Halide pipeline before the smaller result buffer is transferred back to the CPU. This drastically reduces the amount of data transferred over the PCIe bus compared to transferring the full-resolution image and downsampling it on the CPU, leading to smoother UI interactions.
  * **Quality:** The CPU-side implementation (`WorkingImageCPU`) was also refined to use `OIIO::ImageBufAlgo::resize` (Lanczos3 filter) instead of `OIIO::ImageBufAlgo::resample` (bilinear), improving the visual quality of CPU-based downsampling.

---

### Introduction of Common Image Types

- **Problem:** Inconsistent types (`int`, `size_t`, `unsigned int`, etc.) were used throughout the codebase for representing image dimensions (width, height), channel counts, pixel coordinates (x, y), and buffer sizes. This led to potential confusion, implicit narrowing conversions, casting errors, and reduced type safety.
- **Solution:** A new header `Core::Common::ImageTypes` was introduced to define specific, semantically clear type aliases:
  * `ImageDim` for dimensions (width, height) - aliased to `std::size_t` to support very large images (e.g., 64MP+).
  * `ImageChan` for channel count - aliased to `std::uint8_t` (sufficient for up to 255 channels, typically 3 or 4).
  * `ImageCoord` for pixel coordinates (x, y) - aliased to `std::int32_t` to allow for negative offsets in region calculations.
  * `ImageSize` for data size (element count) - aliased to `std::size_t`.
- **Impact:**
  * **Type Safety:** Using these dedicated types makes the code more robust by preventing accidental mixing of dimensions with coordinates or channel counts.
  * **Code Clarity:** Function signatures and member variable declarations are now more self-documenting, clearly indicating the purpose of each numeric value.
  * **Consistency:** Across the Core library, classes like `ImageRegion`, `IWorkingImageHardware`, `StateImageManager`, `WorkingImageContext`, `OperationDescriptor`, and many others have been systematically updated to use these new types for member variables, constructor parameters, method arguments, and return values. For example, `WorkingImageGPU_Halide::downsample` now explicitly expects `Common::ImageDim` for its target dimensions.

---

### Refinement of `ImageRegion` Structure

- **Problem:** The `ImageRegion` structure, being a core data type, needed to be safer, more efficient, and easier to use for common operations like accessing dimensions, validating data integrity, and interfacing with external libraries like Halide.
- **Solution:** Several targeted improvements were made to `Core::Common::ImageRegion`:
  * **Safe Accessors:** Added `constexpr` and `noexcept` getter methods (`width()`, `height()`, `channels()`, `format()`, `x()`, `y()`) for retrieving image properties. These are marked `[[nodiscard]]` and can be evaluated at compile-time if possible, providing safe and efficient access without exposing internal member names.
  * **Robust Validation:** The `isValid()` method was significantly enhanced. It now includes checks for potential integer overflow during the calculation of the expected total element count (`width * height * channels`) and verifies that this calculated size matches the actual `m_data.size()`. This prevents errors caused by inconsistent or corrupted image region data.
  * **Efficient Data Access:** The `getBuffer()` method was added, returning `std::span<float>` (both mutable and const versions). This provides a non-owning, efficient view into the pixel data, which is ideal for passing to external libraries like Halide or for internal algorithms without copying the underlying `std::vector<float>`.
  * **Direct Indexing:** An `operator()(int y, int x, int c)` was introduced for convenient direct access to individual pixel values. It includes `assert` statements in Debug builds to catch out-of-bounds access early during development, while remaining fast in Release builds.
  * **Generic Programming:** C++23 `ImageLike` and `MutableImageLike` concepts were defined. These allow generic algorithms to be written that can operate on any type conforming to the `ImageRegion` interface (having `width`, `height`, `channels`, `getBuffer`, etc.), increasing flexibility and reducing code duplication.
- **Impact:** `ImageRegion` is now a more robust, safer, and developer-friendly cornerstone of the image processing pipeline, facilitating better performance and easier maintenance.

---

### Renaming and Refactoring in Processing Components

- **`WorkingImageContext::downsample` renamed:** The method `downsample` within `WorkingImageContext` was renamed to `getDownsampled`. While still performing a computation, this name better reflects its role as a provider of a new, downsampled image region, aligning with the common getter naming convention.
- **`WorkingImageCPU::downsample` optimization:** The CPU-side downsampling implementation in `WorkingImageCPU::downsample` was updated. It now utilizes `OIIO::ImageBufAlgo::resize` instead of `OIIO::ImageBufAlgo::resample`. This change leverages a higher-quality filtering algorithm (Lanczos3 by default) instead of simple bilinear interpolation, resulting in visually superior downsampled images on the CPU.

---

### `CaptureMoment::Core::Managers::StateImageManager` (Centralized Management)

* **Ownership of SourceManager:** Now owns `m_source_manager` exclusively, decoupling `PhotoEngine` from direct I/O concerns.
* **Delegated Responsibilities:** Acts as a coordinator between `SourceManager`, `PipelineContext`, and `WorkerContext`, preparing data and delegating execution.
* **Enhanced Interface:** Provides methods like `loadImage`, `commitWorkingImageToSource`, and getters for source image properties (`width`, `height`, `channels`).

---

### Independent Serialization Layer

The core library includes a flexible system for saving and loading the state of image operations using XMP metadata. This system is designed as an **independent layer**, separate from the core image processing engine (`PhotoEngine`), to maximize modularity and flexibility.

### Components

* **`CaptureMoment::Core::Serializer::IXmpProvider`:** An interface abstracting the low-level XMP packet read/write operations. This allows switching between different XMP libraries (e.g., Exiv2, Adobe XMP Toolkit) without changing dependent code.

* **`CaptureMoment::Core::Serializer::FileSerializerManager`:** The main manager coordinating the serialization process, using strategies for path determination and actual read/write operations.

* **`CaptureMoment::Core::Serializer::IXmpPathStrategy`:** Defines how the XMP file path is determined (e.g., sidecar file, embedded in image file).

* **`CaptureMoment::Core::Serializer::IFileSerializerReader/Writer`:** Interfaces for reading/writing the list of `OperationDescriptor`s from/to the XMP structure.

### Benefits

* **Flexibility:** The serialization layer (`FileSerializerManager`) can be managed and invoked independently by the UI layer (e.g., via `UISerializerManager` in the Qt module) without requiring `PhotoEngine` to hold a reference to it.
* **Maintainability:** Changes to serialization mechanisms or strategies do not impact the core processing engine.
* **Clear Responsibility:** `PhotoEngine` handles image processing state and pipeline execution. A separate service handles persistence.
* [🟦 **SEE SERIALIZER.md**](core/SERIALIZER.md).

---

### Dynamic Binding Correction

- **Dynamic Binding Correction**: Fixed pipeline execution to correctly bind input buffers at runtime, resolving issues with static compilation.

---

### Simplified PhotoEngine Architecture

- **Reduced Coupling:** `PhotoEngine` constructor now has zero parameters, with internal managers handling their own dependencies. It no longer directly owns `SourceManager`.
- **Centralized Management:** `StateImageManager` now owns `m_source_manager`, `m_pipeline_builder`, and `m_operation_factory` for better encapsulation and clearer responsibilities.

---

### The most significant recent evolution is the introduction of a hardware-agnostic processing layer that abstracts CPU and GPU execution behind a unified interface.

### `CaptureMoment::Core::ImageProcessing::IWorkingImageHardware` (Abstract Interface)

This interface represents an image used as a working buffer, abstracting its hardware location (CPU RAM or GPU memory).

* **Key Methods:**
  * `exportToCPUCopy()`: Creates a CPU copy of the image data for display or saving.
  * `updateFromCPU(const ImageRegion&)`: Updates the working buffer from CPU data.
  * `isValid()`: Checks if the buffer is valid.

* **`WorkingImageGPU_Halide`**: Concrete implementation inheriting from `IWorkingImageGPU` and `WorkingImageHalide`. Combines GPU-specific logic (device transfers), raw data management (`WorkingImageData`), and Halide buffer logic (`WorkingImageHalide`).

---

- **Hierarchical Structure:** Concrete implementations like `WorkingImageCPU_Halide` and `WorkingImageGPU_Halide` now inherit from specific base classes (`WorkingImageCPU`/`IWorkingImageGPU`) which themselves inherit from `IWorkingImageHardware` and `WorkingImageData`. They also inherit from `WorkingImageHalide`.
- **Unified Buffer Initialization:** `WorkingImageHalide::initializeHalide` now accepts a `std::span<float>` for safer and more flexible buffer view creation.

---

## 14. Utility Modules and Generic Conversion

* **`CaptureMoment::Core::Utils::ToStringConverter`**: A utility class using templates and `std::format` to provide a generic `toString` function for various types (e.g., enums like `CoreError`, floats, doubles, ints). This replaces multiple hand-written serialization functions within the core, promoting code reuse and consistency.

* **Usage:** Replaces legacy specific functions like `serializeFloat`, `serializeDouble`, etc., within the serialization module and other parts of the core requiring type-to-string conversion.
  