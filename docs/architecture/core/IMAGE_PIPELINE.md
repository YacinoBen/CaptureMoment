# 🧠 Halide Pipeline Architecture: The Chunky-First Paradigm

## Overview

The Halide execution pipeline in CaptureMoment is built around a strict, zero-copy philosophy. Instead of treating memory layout conversions as a necessary evil, the pipeline enforces an Interleaved (Chunky) memory layout from the moment the image is decoded until it is rendered on screen.

This decision fundamentally shapes how the Halide Abstract Syntax Tree (AST) is scheduled for both CPU and GPU targets, requiring explicit stride constraints and specific loop reorderings to maintain high performance without ever copying or shuffling pixel data.The Halide execution pipeline in CaptureMoment is built around a strict, zero-copy philosophy. Instead of treating memory layout conversions as a necessary evil, the pipeline enforces an Interleaved (Chunky) memory layout from the moment the image is decoded until it is rendered on screen.

This decision fundamentally shapes how the Halide Abstract Syntax Tree (AST) is scheduled for both CPU and GPU targets, requiring explicit stride constraints and specific loop reorderings to maintain high performance without ever copying or shuffling pixel data.

---

## The Memory Layout Dilemma: Chunky vs. Planar
To understand the pipeline architecture, one must understand the two primary ways to store multi-channel images in memory:

* **Planar (Separated):** [R0, R1, R2... G0, G1, G2... B0, B1, B2...]
* Pros: Trivial vectorization for single-channel algorithms (e.g., blurring only the Red channel). This is the default assumption of the Halide scheduling API.
* Cons: Requires explicit conversion to be displayed.

* **Chunky (Interleaved):** [R0, G0, B0, A0, R1, G1, B1, A1...]
* Pros: Native format for display APIs (OpenGL, Vulkan, QML).
* Cons: Harder to vectorize naively if the scheduler treats dimensions independently.

### The Architectural Decision: Why Chunky-First?

In a real-time photo editing UI, memory bandwidth and PCIe transfers are critical bottlenecks. Every time an image is zoomed, panned, or a slider is adjusted, a downscaled preview must be generated and pushed to the GPU for rendering.

If we used Planar memory for processing, the pipeline would look like this:

1. Decode JPEG to Chunky.
2. Convert to Planar (Expensive CPU cache-miss intensive loop).
3. Execute Halide algorithm.
4. Convert back to Chunky (Another expensive loop).
5. Upload to GPU for display.

By enforcing a Chunky-First pipeline, we eliminate steps 2 and 4. The Halide pipeline reads the Chunky buffer directly, modifies it, and the result is instantly ready for the UI thread. Even as the pipeline evolves to include heavier computational algorithms (such as complex color space transformations, advanced noise reduction, or OpenCV integration), avoiding these continuous memory channel shuffles for UI feedback remains a massive net gain for responsiveness.

---

## Enforcing Chunky in Halide: The Technical Implementation
By default, Halide's scheduling API assumes a Planar layout (Stride X = 1). To force the compiler to generate correct machine code for Chunky data, a three-layer constraint system is applied to every pipeline graph.

### 1. The Physical Buffer (C++ API)
When wrapping the `std::span<float>` from the source manager, we do not use the default Halide buffer constructor. We use:

`Halide::Buffer<float>::make_interleaved(data_ptr, width, height, channels)`;
This configures the physical memory strides (X=4, Y=Width*4, C=1) without allocating or moving a single byte.

```cpp
m_input.dim(0).set_stride(4); 
m_input.dim(2).set_stride(1);
```

### 3. The Output Contract (OutputImageParam)
This is the most critical and easily missed step. Even though a `Halide::Func` is a pure mathematical object, its output realization has an implicit constraint (Stride X = 1). If not explicitly overridden, Halide will crash at runtime (`Constraint violated`) when the provided output buffer is Chunky.

```cpp
output_func.output_buffer().dim(0).set_stride(4);
output_func.output_buffer().dim(2).set_stride(1);
```

---

## Scheduling Strategies for Chunky Data
Because the memory layout is Chunky, the default Halide schedules (which vectorize over X) are highly suboptimal or simply crash. The scheduling must be explicitly adapted for the interleaved strides.

### CPU Scheduling: Contiguous Vectorization
The default loop order `Y -> X -> C` is disastrous for Chunky data on a CPU. When the inner loop is over `X`, the CPU tries to load `R0`, but the next pixel `R1` is 4 bytes away. This forces LLVM to generate expensive "Gather" instructions instead of fast contiguous "Load" instructions.

We solve this by forcing the channel `C` to be the innermost loop, matching the physical memory layout:

```cpp
pipeline.bound(c, 0, 4)       // Guarantee exactly 4 channels (RGBA)
         .reorder(c, x, y)    // Loop order is now Y -> X -> C
         .split(y, yo, yi, 32)
         .parallel(yo)
         .unroll(c);           // Unroll R, G, B, A
```
### GPU Scheduling: Thread-Level Constraints
On the GPU, `gpu_tile(x, y, ...)` maps `x` and `y` to GPU threads. We cannot reorder c outside the thread bounds without breaking the parallelism.

Instead, the absolute priority is satisfying the stride constraints. If the GPU backend attempts to vectorize the `x` dimension natively and encounters a stride of 4, it will attempt an unsupported scatter/gather operation, resulting in a silent driver crash (SIGABRT).

The solution is to leave the channel loop implicit inside the GPU threads, relying purely on the stride constraints to guide the backend compiler (CUDA/Vulkan) to generate correct memory access patterns for the `RGBA` layout without crashing.

```cpp
// No reorder on GPU. Just strict stride constraints + tiling.
pipeline.gpu_tile(x, y, xo, yo, xi, yi, 16, 16);
```