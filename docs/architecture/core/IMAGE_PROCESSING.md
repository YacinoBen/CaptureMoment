# 🖼️ IMAGE PROCESSING ARCHITECTURE

## Overview

The image processing architecture in CaptureMoment is designed to be **hardware-agnostic**, **high-performance**, and **non-destructive**. It abstracts the underlying hardware (CPU or GPU) behind clean interfaces, allowing the same processing pipeline to run efficiently on different platforms without code changes.

The core innovation revolves around the `IWorkingImageHardware` interface and the `WorkingImageContext` lifecycle manager. Data ownership is centralized in `WorkingImageData`, while hardware workers operate on zero-copy `ImageView` spans. This separation enables explicit memory control, pipeline pre-compilation, and instant non-destructive resets without re-reading source files.

---

## Key Components

### 1. `IWorkingImageHardware` (Abstract Interface)

This is the central abstraction that enables hardware-agnostic processing. It defines the contract for interacting with an image buffer, regardless of its physical location.

**Key Methods:**
- `virtual std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError> getFullResImage() const = 0;`
  - Creates a deep CPU copy of the full-resolution processed image data for display, saving, or committing to source.
- `virtual bool bindView(const ImageView& view) = 0;`
  - Attaches a non-owning view of image data (working span, original span, and dimensions) to the hardware worker. Enables zero-copy processing.
- `virtual bool isValid() const = 0;`
  - Checks if the buffer/view is valid and ready for processing.
- `virtual std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError> downsample(Common::ImageDim target_width, Common::ImageDim target_height) = 0;`
  - Exports a downscaled version of the image. GPU implementations perform this directly on the device before transferring the small result, minimizing PCIe bandwidth.

### 2. `ImageView` (Zero-Copy View)

A lightweight, non-owning structure that bridges `WorkingImageContext` (data owner) and hardware workers.

```cpp
struct ImageView {
    std::span<float> working_data;        ///< Mutable span for processing
    std::span<const float> original_data; ///< Const span for non-destructive reset
    Common::ImageDim width{0};
    Common::ImageDim height{0};
    Common::ImageChan channels{0};
};
```

- **Purpose:** Eliminates data duplication. Hardware backends bind to these spans once during initialization, and all subsequent operations read/write directly to the shared memory.

### 3. Base Classes

#### `WorkingImageData`

- **Purpose:** Owns the CPU RAM buffers and manages image metadata.

- **Responsibilities:**
  - Maintains `std::vector<float> m_data` (working buffer) and `std::vector<float> m_original_data` (source cache).
  - Provides `initializeData(Common::ImageRegion&&)` for setup and `restoreOriginalData()` (via `std::ranges::copy`) for instant non-destructive resets.
  - Exposes accessors: `getWorkingDataSpan()`, `getOriginalDataSpan()`, `getWidth()`, `getHeight()`, `getChannels()`, `isValid()`

#### `WorkingImageHalide`  (Shared Halide Logic)

- **Purpose:**Provides common Halide buffer functionality for both CPU and GPU implementations.
- **Responsibilities:**
  - Holds `Halide::Buffer<float>`.
  - `initializeHalide(std::span<float>, ...)` creates a zero-copy Halide view over external memory.
  - `isHalideBufferValid()` checks buffer definition state.

### 4. Concrete Implementations

#### `WorkingImageCPU` & `WorkingImageCPU_Halide`

- **Inherits From:** `WorkingImageCPU_Halide` → `WorkingImageCPU` → `IWorkingImageHardware` + `WorkingImageData` + `WorkingImageHalide`
- **Purpose:**  Default fallback backend. Data is bound directly to CPU RAM via `bindView()`.

**Key Additions:**
  - `getOriginalHalideBuffer()` & `isOriginalHalideBufferValid()`: Expose the original data buffer for non-destructive pipeline execution.
  - CPU scheduling uses optimized split/parallel/vectorize directives.

#### `WorkingImageCPU_Halide` & `WorkingImageGPU_Halide`
- **Inherits From:** `WorkingImageGPU_Halide` → `WorkingImageGPU` → `IWorkingImageHardware` + `WorkingImageData` + `WorkingImageHalide`
- **Purpose:** High-performance GPU backend with explicit VRAM management.
**Key Additions:**
  - `transferToVRAM()`: Pre-compiles and caches Halide pipelines (`m_reset_pipeline`, `m_downsample_pipeline`) for GPU execution.
  - `resetExecutionBuffer()`: Restores GPU working buffer from original buffer via cached pipeline.
  - `getExecutionBuffer()`: Returns reference to GPU buffer for direct pipeline realization.
  - `syncToHostRAM()`: Forces explicit GPU→CPU synchronization.
  - `downloadDeviceToHost()`: Protected method for backend-specific transfer logic.
  - `downsample`: Uses pre-compiled Catmull-Rom cubic resampling pipeline with GPU tiling.

### 5. Lifecycle & Factory Management

#### `WorkingImageFactory` (Registry Pattern)

- **Responsibility:** Creates empty hardware abstractions based on `AppConfig` backend settings.
- **Signature Change:** Creator functions are now parameterless: `std::function<std::unique_ptr<IWorkingImageHardware>()>`.
- **Impact:** Decouples object instantiation from data management. The factory returns an uninitialized worker; data binding happens later via `bindView()`.


#### `WorkingImageContext` (Context Pattern)
- Responsibility: Central coordinator for the image processing lifecycle.
- Workflow in `prepare()`:
  1. Creates `WorkingImageData` (owns CPU RAM).
  2. Creates hardware worker via ``WorkingImageFactory`.
  3. Constructs  `ImageView` from owned data.
  4. Calls  `worker->bindView(view)` to attach zero-copy spans.
- Non-Destructive Reset: `resetToOriginal()` calls `WorkingImageData::restoreOriginalData()`. Changes are instantly visible to the bound hardware worker via shared `std::span`.

### 6. Pipeline Execution Architecture
#### `OperationPipelineExecutor` (Abstract Base)
- **Role:** Defines contract for fused Halide pipeline execution.
- **Key Methods:**
  - **init(operations, factory)**: Builds and compiles the fused graph.
  - **updateRuntimeParams(operations)**: Updates `Halide::Param<float>` values without recompilation.
  - **executeOnHalideBuffer(input, output)**: Executes pipeline with separate input and output buffers, enabling non-destructive processing and dimension-changing operations.

#### Backend-Specific Executors
- **OperationPipelineExecutorCPU:** Applies CPU scheduling (`split(y, yo, yi, 32).parallel(yo).vectorize(x, 8)`). Retrieves original/working buffers from `WorkingImageCPU_Halide`.
- **OperationPipelineExecutorGPU:** Applies GPU scheduling via `gpu_tile()` when `target.has_gpu_feature()` is true. Calls `resetExecutionBuffer()` before execution.
- **Selection:** `PipelineRegistry::registerHalideExecutors()` queries `AppConfig::getProcessingBackend()` at startup and registers the appropriate executor with PipelineBuilder.


### 7. Backend Selection System
#### `AppConfig` (**Singleton**)
Stores the selected backend globally: **CPU_RAM** or **GPU_MEMORY**.

#### `BenchmarkingBackendDecider`
Performs runtime benchmarking at application startup to determine optimal backend:
- Tests supported backends against CPU baseline.
- Applies a performance margin to avoid GPU overhead for negligible gains.
- Stores result in `AppConfig` for the entire lifecycle.

### 8. UI Integration & Performance Considerations
The UI layer remains completely unaware of hardware specifics:
- **PhotoEngine::getWorkingImageAsRegion():** Calls `getFullResImage()` on the active hardware worker to export processed data to CPU.
- **DisplayManager:** Receives `ImageRegion` objects, handles zoom/pan, and preferentially requests downscaled previews via `IWorkingImageHardware::downsample()`.
- **Rendering Items:** Work exclusively with CPU ImageRegion data.

#### Performance Benefits (UI)
GPU `downsample()` executes entirely on the device, transferring only the small result buffer to RAM. This drastically reduces PCIe traffic and enables smooth, real-time zoom/pan interactions even with 50MP+ source files. CPU `downsample()` uses `OIIO::ImageBufAlgo::resample` with optimized tiling for quality and speed.

### 9. Extensibility Guide
#### Adding a New Backend
1. Create a new class inheriting from `IWorkingImageHardware` (and optionally `WorkingImageData`/`WorkingImageHalide`).
2. Implement `bindView()`, `getFullResImage()`, `isValid()`, and `downsample()` on GPU.
3. Register a creator lambda in `WorkingImageRegistration::registerDefaultBackends()`.
4. Update `BenchmarkingBackendDecider` to include it in priority order.

#### Adding a New Operation
1. Implement `IOperationFusionLogic` and define `appendToFusedPipeline()`.
2. Register in `OperationRegistry::registerAll()`.
3. The fused pipeline system automatically integrates it without modifying executor logic.


## GPU Backend Requirements

| Backend |    Platform  | Requirements | Runtime |
|:------------:|:--------------:|:----------:|:----------:|
| CUDA      | NVIDIA GPUs    | CUDA Toolkit installed, NVIDIA drivers   |   No |
| OpenCL        | AMD/NVIDIA/Intel    | OpenCL runtime (usually included with GPU drivers)  | Yes |
| Vulkan      | Cross-platform    | Vulkan SDK and compatible driver    | Yes |
| DirectX12  | Windows only          | Windows 10/11, compatible GPU      |  No |
| Metal  | Apple platforms only          | macOS 10.11+/iOS 8.0+, compatible GPU |  No| 
| OpenGL  | Cross-platform (Desktop)          | OpenGL 4.3+ compatible driver| Yes |
