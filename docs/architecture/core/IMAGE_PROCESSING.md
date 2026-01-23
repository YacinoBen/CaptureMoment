# 🖼️ IMAGE PROCESSING ARCHITECTURE

## Overview

The image processing architecture in CaptureMoment is designed to be **hardware-agnostic**, **high-performance**, and **extensible**. It abstracts the underlying hardware (CPU or GPU) behind clean interfaces, allowing the same processing pipeline to run efficiently on different platforms without code changes.

The core innovation is the `IWorkingImageHardware` interface, which represents an image buffer that can reside in CPU RAM or GPU memory. All operations and pipelines interact with this interface, making the hardware location transparent to the processing logic.

## Key Components

### 1. `IWorkingImageHardware` (Abstract Interface)

This is the central abstraction that enables hardware-agnostic processing.

**Key Methods:**
- `virtual std::unique_ptr<Common::ImageRegion> exportToCPUCopy() const = 0;`
  - Creates a CPU copy of the image data for display, saving, or debugging.
- `virtual bool updateFromCPU(const Common::ImageRegion& cpu_region) = 0;`
  - Updates the working buffer from CPU data (used for initialization and loading).
- `virtual bool isValid() const = 0;`
  - Checks if the buffer is valid and ready for processing.
- `virtual std::pair<size_t, size_t> getSize() const = 0;`
  - Returns the width and height of the image.
- `virtual size_t getChannels() const = 0;`
  - Returns the number of channels (e.g., 4 for RGBA).
- `virtual size_t getDataSize() const = 0;`
  - Returns the total number of elements (width × height × channels).

### 2. Concrete Implementations

#### `WorkingImageCPU_Halide`

- **Backend:** CPU using Halide buffers.
- **Storage:** `Halide::Buffer<float>` stored in CPU RAM.
- **Performance:** Optimized for CPU parallelism using Halide's vectorization and tiling.
- **Use Case:** Default backend, fallback when GPU is unavailable or slower.

#### `WorkingImageGPU_Halide`

- **Backend:** GPU using Halide with GPU scheduling.
- **Storage:** `Halide::Buffer<float>` stored in GPU memory.
- **Performance:** Leverages GPU parallelism for compute-intensive operations.
- **Use Case:** Primary backend when GPU benchmarking shows performance advantage.

### 3. `WorkingImageFactory` (Factory Pattern)

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

### 4. Backend Selection System
#### MemoryType (Enum)
Defines the available memory types:

* **CPU_RAM:** Process on CPU.
* **GPU_MEMORY:** Process on GPU.

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

#### Benchmark Logic:

* Tests each supported backend in priority order.
* Compares execution time against CPU baseline.
* Applies a 10% margin to favor CPU when differences are negligible (to avoid GPU memory transfer overhead).
* Stores the result in AppConfig for the entire application lifecycle.

### 5. Integration with Processing Pipeline
#### IOperation::execute()

Operations accept **IWorkingImageHardware&**
```cpp
virtual bool execute(
    ImageProcessing::IWorkingImageHardware& working_image,
    const OperationDescriptor& params
) = 0;
```

#### Implementation Strategy:

Operations work directly with the hardware buffer when possible (e.g., casting to **WorkingImageCPU_Halide** for Halide-specific optimizations).
Fall back to **exportToCPUCopy()** and **updateFromCPU()** when direct access isn't available or necessary.

#### OperationPipeline::applyOperations()

The pipeline orchestrates operations on the hardware-agnostic buffer:

```cpp
static bool applyOperations(
    ImageProcessing::IWorkingImageHardware& working_image,
    const std::vector<OperationDescriptor>& operations,
    const OperationFactory& factory
);
```

### 6.Pipeline Fusion Architecture
#### WorkingImageHalide Base Class
* **Shared Infrastructure:** Base class providing common Halide buffer functionality for both CPU and GPU implementations.
* **Memory Management:** Uses **std::vector<float>** as backing store for Halide buffers, enabling in-place modifications without unnecessary copies.
* **Direct Buffer Access:** Provides **getHalideBuffer()** method for direct pipeline execution.
#### IPipelineExecutor & OperationPipelineExecutor

* **IPipelineExecutor:** Abstract interface for executing a pre-built pipeline on an image.
* **OperationPipelineExecutor:** Concrete implementation that executes fused adjustment operation pipelines using Halide's computational graph optimization.

#### OperationPipelineBuilder
* Builds and compiles the fused Halide pipeline for the stored operations.
* **Optimization:** Creates combined computational passes that eliminate intermediate buffer copies between operations.
* **Integration:** Works with **IOperationFusionLogic** implementations to chain operations into a single pipeline.

#### IOperationFusionLogic Interface
* **appendToFusedPipeline:** Operations implement this interface to provide their fusion logic for combining operations into a single computational graph.
* **Sequential vs Fused:** Operations maintain both execute (for sequential processing) and **appendToFusedPipeline** (for pipeline fusion) methods.
* **[[maybe_unused]]:** Sequential execute methods are marked as unused when primarily using fused execution.

### 7. UI Integration
The UI layer remains completely unaware of the hardware abstraction:

* **PhotoEngine::getWorkingImageAsRegion():** Exports the current working image to CPU for display.
* **DisplayManager:** Receives **ImageRegio**n objects and handles downsampling/zoom/pan.
* **Rendering Items:** Work exclusively with CPU data (**ImageRegion**).
This ensures the UI code stays simple and focused on presentation logic.

### 8. Performance Considerations
#### Memory Transfer Overhead
* GPU processing involves copying data from CPU to GPU (**updateFromCPU**) and back (**exportToCPUCopy**).
* The benchmarking system accounts for this overhead when deciding the optimal backend.
* For small images or simple operations, CPU is often faster due to avoiding transfer costs.

#### Halide Optimization
* Both CPU and GPU implementations use Halide for optimal code generation.
* CPU version uses vectorization and multi-threading.
* GPU version uses tiling and GPU-specific scheduling directives.

#### Pipeline Fusion Performance
* **Zero-Copy Processing:** **WorkingImageHalide** base class eliminates unnecessary data copying by sharing memory between **std::vector<float>** and **Halide::Buffer**.
* **In-Place Processing:** Halide buffers operate directly on shared data vectors, eliminating redundant copies.
* **Optimized Scheduling:** Pipeline fusion creates single computational passes instead of multiple sequential operations.
* ***Hardware Agnostic:** Same fusion logic works for both CPU and GPU backends through the unified interface.

#### Benchmarking Accuracy
* Benchmarks use representative operations (brightness/contrast) that simulate real-world usage.
* Multiple runs with deterministic data ensure consistent results.
* Results are cached in **AppConfig** to avoid repeated benchmarking.


#### Extensibility
##### Adding New Backends
To add a new backend (e.g., CUDA native, OpenCV):

* Create a new class **implementing IWorkingImageHardware**.
* Add the backend to **WorkingImageFactory::create()**.
* Update **BenchmarkingBackendDecider** to include the new backend in its priority order.
* Implement the **execute()** method in operations to handle the new backend type.

##### Operation Implementation
Operations can choose their level of backend integration:

* Generic **Approach:** Use **exportToCPUCopy()** and **updateFromCPU()** for maximum compatibility.
* **Optimized Approach:** Cast to specific implementations (e.g., *WorkingImageCPU_Halide*) for direct buffer access and performance.

#### Build Requirements
##### Halide Version
* **Minimum Version:** Halide 21.0.0

##### GPU Backend Requirements


| Backend |    Platform  | Requirements |
|:------------:|:--------------:|:----------:|
| CUDA      | NVIDIA GPUs    | CUDA Toolkit installed, NVIDIA drivers   |   
| OpenCL        | AMD/NVIDIA/Intel    | OpenCL runtime (usually included with GPU drivers)  |
| Vulkan      | Cross-platform    | Vulkan SDK and compatible driver    |
| DirectX12  | Windows only          | Windows 10/11, compatible GPU      | 
| Metal  | Apple platforms only          | macOS 10.11+/iOS 8.0+, compatible GPU | 
| OpenGL  | Cross-platform (Desktop)          | OpenGL 4.3+ compatible driver| 
