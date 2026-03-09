# 🖼️ IMAGE PROCESSING ARCHITECTURE

## Overview

The image processing architecture in CaptureMoment is designed to be **hardware-agnostic**, **high-performance**, and **extensible**. It abstracts the underlying hardware (CPU or GPU) behind clean interfaces, allowing the same processing pipeline to run efficiently on different platforms without code changes.

The core innovation is the `IWorkingImageHardware` interface, which represents an image buffer that can reside in CPU RAM or GPU memory. All operations and higher-level pipelines interact with this interface, making the hardware location transparent to the processing logic.

## Key Components

### 1. `IWorkingImageHardware` (Abstract Interface)

This is the central abstraction that enables hardware-agnostic processing.

**Key Methods:**
- `virtual std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError> exportToCPUCopy() const = 0;`
  - Creates a CPU copy of the image data for display, saving, or debugging.
- `virtual std::expected<void, ErrorHandling::CoreError> updateFromCPU(const Common::ImageRegion& cpu_region) = 0;`
  - Updates the working buffer from CPU data (used for initialization and loading).
- `virtual bool isValid() const = 0;`
  - Checks if the buffer is valid and ready for processing.
- `virtual std::pair<size_t, size_t> getSize() const = 0;`
  - Returns the width and height of the image.
- `virtual size_t getChannels() const = 0;`
  - Returns the number of channels (e.g., 4 for RGBA).
- `virtual size_t getDataSize() const = 0;`
  - Returns the total number of elements (width × height × channels).
- `virtual std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError> downsample(size_t target_width, size_t target_height) = 0;`
  - Exports a *downscaled* version of the image directly from the hardware buffer. This is the **preferred method for display purposes** as it avoids transferring large amounts of data when only a smaller preview is needed, often performing the downsampling *on the GPU* before transfer.

### 2. Base Classes

#### `WorkingImageData`

- **Purpose:** Base class providing raw pixel data storage (`std::unique_ptr<float[]>`) and metadata (width, height, channels, validity state `m_valid`) for all working image implementations.
- **Responsibilities:**
  - Manages the underlying pixel data buffer using `std::unique_ptr<float[]>` allocated via `std::make_unique_for_overwrite` to avoid zero-initialization overhead.
  - Stores image dimensions and channel count.
  - Maintains a `m_valid` flag indicating the state of the raw data.
  - Provides `initializeData` for buffer setup and `getDataSpan` for safe access via `std::span`.

#### `WorkingImageHalide`

- **Purpose:** Base class providing shared Halide buffer (`Halide::Buffer<float>`) logic for both CPU and GPU implementations.
- **Responsibilities:**
  - Holds the `Halide::Buffer<float>` object.
  - Provides `initializeHalide(std::span<float>, ...)` to create a zero-copy view of external data obtained from `WorkingImageData`.
  - Offers specific getters for dimensions/channels based on the Halide buffer (`getSizeByHalide`, `getChannelsByHalide`, etc.).

### 3. Concrete Implementations

#### `WorkingImageCPU` (Concrete Base Class)

- **Inherits From:** `IWorkingImageHardware`, `WorkingImageData`
- **Purpose:** Concrete base class for CPU-specific implementations, combining the hardware interface with raw data management.

#### `WorkingImageCPU_Halide`

- **Inherits From:** `WorkingImageCPU`, `WorkingImageHalide`
- **Backend:** CPU using Halide buffers.
- **Storage:** Raw data managed by `WorkingImageData`, Halide buffer view managed by `WorkingImageHalide`.
- **Performance:** Optimized for CPU parallelism using Halide's vectorization and tiling.
- **Use Case:** Default backend, fallback when GPU is unavailable or slower.

#### `IWorkingImageGPU` (Abstract Interface)

- **Inherits From:** `IWorkingImageHardware`, `WorkingImageData`
- **Purpose:** Abstract interface for GPU-specific implementations, combining the hardware interface with raw data management.

#### `WorkingImageGPU_Halide`

- **Inherits From:** `IWorkingImageGPU`, `WorkingImageHalide`
- **Backend:** GPU using Halide with GPU scheduling.
- **Storage:** Raw data managed by `WorkingImageData`, Halide buffer view managed by `WorkingImageHalide`. The buffer is scheduled to reside in GPU memory.
- **Performance:** Leverages GPU parallelism for compute-intensive operations.
- **Use Case:** Primary backend when GPU benchmarking shows performance advantage.
- **`downsample` Implementation:** Performs the downsampling operation directly on the GPU using a dedicated Halide pipeline before transferring only the smaller result to the CPU.

### 4. `WorkingImageFactory` (Factory Pattern)

This factory encapsulates the creation logic for `IWorkingImageHardware` instances.

**Key Method:**
```cpp
static std::unique_ptr<IWorkingImageHardware> create(
    Common::MemoryType backend,
    const Common::ImageRegion& source_image
);
```


**Benefits:**
* **Single Responsibility:** Centralizes creation logic.
* **No Duplication:** Both StateImageManager and PhotoTask use the same factory.
* **Easy Testing:** Mock implementations can be injected for unit tests.

### 5. Backend Selection System
**MemoryType (Enum)**. Defines the available memory types:
* CPU_RAM: Process on CPU.
* GPU_MEMORY: Process on GPU.

#### AppConfig (Singleton)
Stores the selected backend globally:
```cpp
class AppConfig {
public:
    static AppConfig& instance();
    void setProcessingBackend(Common::MemoryType backend);
    Common::MemoryType getProcessingBackend() const;
};
```

#### BenchmarkingBackendDecider
Performs runtime benchmarking to determine the optimal backend:
Priority Order:
* **Hardware-Specific:** CUDA (for NVIDIA GPUs)
* **OS-Specific:** DirectX12 (Windows), Metal (macOS)
* **Cross-Platform:** Vulkan (Windows, Linux, Android)
* **Fallback/Legacy:** OpenCL (widely supported but often slower)

##### Benchmark Logic:
Tests each supported backend in priority order.
Compares execution time against CPU baseline.
Applies a 10% margin to favor CPU when differences are negligible (to avoid GPU memory transfer overhead).
Stores the result in AppConfig for the entire application lifecycle.

### 6. Integration with Higher-Level Components
**Operation Execution:**

Operations accept **IWorkingImageHardware&** the concrete implementations (e.g.,**WorkingImageCPU_Halide**,**WorkingImageGPU_Halide**) to perform their calculations, often leveraging the **Halide::Buffer<float>** obtained via **getHalideBuffer()** (which comes from **WorkingImageHalide**).

**Pipeline Integration:**
Higher-level pipeline executors (like **OperationPipelineExecutor**) receive an **IWorkingImageHardware&**, perform a dynamic_cast to determine the concrete type (e.g., **WorkingImageCPU_Halide**** or **WorkingImageGPU_Halide***), and then call methods like isValid, getSize, getChannels on the concrete object. They then cast the object to WorkingImageHalide& to access getHalideBuffer() for binding to the Halide pipeline.

This interaction relies heavily on the methods defined in WorkingImageHalide and WorkingImageData, demonstrating how the architecture separates data management and Halide-specific logic into reusable base classes.


### 7. UI Integration
The UI layer remains completely unaware of the hardware abstraction:
* **PhotoEngine::getWorkingImageAsRegion():** Exports the current working image to CPU for display using exportToCPUCopy() or the optimized downsample() method.
* **DisplayManager:** Receives ImageRegion objects and handles downsampling/zoom/pan. It can now preferentially use downsample() from IWorkingImageHardware for efficiency.
* **Rendering Items:** Work exclusively with CPU data (ImageRegion).
This ensures the UI code stays simple and focused on presentation logic.
#### Performance Considerations (UI):
The downsample method on IWorkingImageHardware implementations (especially WorkingImageGPU_Halide) performs the downsampling operation on the hardware (GPU) before transferring only the smaller result to the CPU, significantly reducing transfer time for display previews.

#### Extensibility
##### Adding New Backends
To add a new backend (e.g., CUDA native, OpenCV):
* Create a new class implementing **IWorkingImageHardware**.
* Consider inheriting from **WorkingImageData** and/or **WorkingImageHalide** if the new backend shares similar data management or Halide integration patterns.
* Add the backend to **WorkingImageFactory::create()**.
* Update BenchmarkingBackendDecider to include the new backend in its priority order.

##### Operation Implementation
Operations can choose their level of backend integration:
* **Generic Approach:** Use **exportToCPUCopy()** and **updateFromCPU()** for maximum compatibility.
* **Optimized Approach:** Cast to specific implementations (e.g., **WorkingImageCPU_Halide**, **WorkingImageGPU_Halide**) for direct buffer access and performance, leveraging the **Halide::Buffer<float>** provided by **WorkingImageHalide**.


## GPU Backend Requirements

| Backend |    Platform  | Requirements | Runtime |
|:------------:|:--------------:|:----------:|:----------:|
| CUDA      | NVIDIA GPUs    | CUDA Toolkit installed, NVIDIA drivers   |   No |
| OpenCL        | AMD/NVIDIA/Intel    | OpenCL runtime (usually included with GPU drivers)  | Yes |
| Vulkan      | Cross-platform    | Vulkan SDK and compatible driver    | Yes |
| DirectX12  | Windows only          | Windows 10/11, compatible GPU      |  No |
| Metal  | Apple platforms only          | macOS 10.11+/iOS 8.0+, compatible GPU |  No| 
| OpenGL  | Cross-platform (Desktop)          | OpenGL 4.3+ compatible driver| Yes |
