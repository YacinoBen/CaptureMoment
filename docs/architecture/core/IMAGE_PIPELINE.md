# 🧠 Halide Pipeline Architecture Guidelines: Adaptive Memory Layout Strategy

## 📋 Purpose of This Document

This document serves as a **conceptual guideline** for understanding the memory layout strategy used in CaptureMoment's Halide processing pipeline. It explains **why** we use different memory layouts for CPU and GPU backends, **what** Chunky and Planar formats mean, and **how** this design enables high-performance, zero-copy image processing.


---

## 🎯 Core Principle: Zero-Copy at Boundaries, Optimized Layout Internally

The pipeline enforces a **Chunky (interleaved) memory layout at all I/O boundaries**:
* Input from OpenImageIO → Chunky `[R,G,B,A]`
* Output to Display/Export → Chunky `[R,G,B,A]`

**However, internally**, the pipeline adapts its memory layout to maximize hardware-specific performance:
* **CPU backend**: Uses a **transient Planar transposition** during mathematical evaluation to enable efficient SIMD vectorization.
* **GPU backend**: Maintains a **pure Chunky layout** throughout to leverage memory coalescing and hardware thread mapping.

This hybrid approach ensures:
* ✅ Zero-copy handoff at boundaries (no expensive memory shuffles)
* ✅ Optimal execution patterns for each hardware backend
* ✅ Consistent, predictable behavior across platforms

---

## 🧩 Memory Layout Fundamentals

### What is Planar Layout?


Planar: [R0, R1, R2, R3... | G0, G1, G2, G3... | B0, B1, B2, B3...]
         └─  Red channel ─┘ └─ Green channel ─┘ └─ Blue channel ─┘

* Each color channel is stored in a separate, contiguous memory block.
* **Advantages**:
  * Trivial vectorization for single-channel algorithms (e.g., blur only the Red channel).
  * Matches Halide's default scheduling assumptions.
* **Disadvantages**:
  * Requires explicit conversion to/from Chunky for display APIs.
  * Incurs two expensive memory shuffles per frame if used end-to-end.

### What is Chunky (Interleaved) Layout?
Chunky: [R0, G0, B0, A0, R1, G1, B1, A1, R2, G2, B2, A2...]
         └── Pixel 0 ──┘ └── Pixel 1 ──┘ └── Pixel 2 ──┘

* All channels for a single pixel are stored together, then the next pixel, and so on.
* **Advantages**:
  * Native format for display APIs (OpenGL, Vulkan, Qt Quick, QML).
  * Zero-copy handoff to UI rendering layer.
  * Matches the output of most image decoders (OpenImageIO, libjpeg, etc.).
* **Disadvantages**:
  * Harder to vectorize naively if the scheduler treats dimensions independently.
  * Requires explicit stride constraints in Halide to generate correct code.

---

## ⚙️ Why Different Layouts for CPU vs GPU?

### CPU Backend: Transient Planar for Vectorization

**Problem**: On CPU, Halide's default vectorizer assumes contiguous memory along the innermost loop. For Chunky data, iterating over `x` while interleaving channels causes scattered memory access, forcing the compiler to generate expensive "Gather" instructions instead of fast contiguous "Load" instructions.

**Solution**: The CPU executor explicitly transposes the input to Planar layout **only for the mathematical evaluation phase**, then transposes it back for output.

**Why this works**:
* The transient Planar layout aligns channels contiguously in memory during mathematical evaluation.
* Enables loop reordering and unrolling strategies that generate tight SIMD code (AVX2, NEON).
* Halide optimizes the double transpose into a single layout transformation pass during JIT compilation, minimizing overhead.

### GPU Backend: Direct Chunky for Memory Coalescing

**Problem**: GPU architectures excel at massive parallelism but suffer from memory coalescing penalties when threads access non-contiguous addresses. Transposing would break the `(x, y) → thread` mapping and introduce synchronization overhead.

**Solution**: The GPU executor processes data in Chunky layout from start to finish.

**Why this works**:
* GPU tiling (`gpu_tile`) maps each `(x, y)` block to a GPU thread/warp.
* Each thread naturally processes all 4 channels sequentially, matching the Chunky stride layout.
* Explicit stride constraints guide the backend compiler (Vulkan, CUDA, Metal) to emit vectorized global loads instead of scalar memory transactions.

---

## 🔄 The Processing Flow (Conceptual)

Input Chunky RGB (Linear)
       │
       ▼
┌─────────────────┐
│ Backend Adapter │
├─────────────────┤
│ CPU: Transpose │
│ Chunky → Planar → Chunky (internal) │
│ GPU: Direct Chunky (no transpose) │
└─────────────────┘
       │
       ▼
┌─────────────────┐
│ Color Conversion│
│ rgbToOklab() │
│ (Alpha preserved)│
└─────────────────┘
       │
       ▼
┌─────────────────┐
│ Fused Operations│
│ applyOperations()│
│ (L-channel only)│
└─────────────────┘
       │
       ▼
┌─────────────────┐
│ Color Conversion│
│ oklabToRgb() │
│ (Alpha preserved)│
└─────────────────┘
       │
       ▼
┌─────────────────┐
│ Backend Adapter │
├─────────────────┤
│ CPU: Transpose │
│ back to Chunky│
│ GPU: Direct Chunky│
└─────────────────┘
       │
       ▼
Output Chunky RGB (Clamped [0,1])


**Key properties**:
* **Alpha passthrough**: The Alpha channel is never modified by color space conversions or operations.
* **L-only adjustments**: All tone operations modify only the `L` channel in Oklab; `a` and `b` remain untouched to prevent hue shifts.
* **Deferred clamping**: No intermediate clamping on `L`; final clamping occurs after `oklabToRgb()` to preserve perceptual accuracy.
* **Zero-copy fusion**: Conversions and operations are fused into a single Halide graph; no intermediate buffers are allocated.

---

## 🛠️ Scheduling Guidelines (Conceptual)

### For CPU Backend

* **Goal**: Enable contiguous SIMD loads by making the channel dimension the innermost loop.
* **Strategy**:
  1. Transpose input from Chunky `(x,y,c)` to Planar `(c,y,x)` for evaluation.
  2. Apply operations with loop order `Y → X → C`.
  3. Unroll the channel loop to generate explicit loads/stores for R, G, B, A.
  4. Transpose back to Chunky `(x,y,c)` for output.
* **Avoid**: Scatter/gather patterns; channel reordering that breaks contiguous access.

#### Contiguous Vectorization
The default loop order `Y -> X -> C` is disastrous for Chunky data on a CPU. When the inner loop is over `X`, the CPU tries to load `R0`, but the next pixel `R1` is 4 bytes away. This forces LLVM to generate expensive "Gather" instructions instead of fast contiguous "Load" instructions.

We solve this by forcing the channel `C` to be the innermost loop, matching the physical memory layout:

```cpp
pipeline.bound(c, 0, 4)       // Guarantee exactly 4 channels (RGBA)
         .reorder(c, x, y)    // Loop order is now Y -> X -> C
         .split(y, yo, yi, 32)
         .parallel(yo)
         .unroll(c);           // Unroll R, G, B, A
```

### For GPU Backend

* **Goal**: Preserve memory coalescing by keeping Chunky layout end-to-end.
* **Strategy**:
  1. Keep input in Chunky `(x,y,c)` throughout.
  2. Apply operations with GPU tiling on `(x,y)` dimensions.
  3. Let each thread handle all 4 channels sequentially.
  4. Enforce explicit stride constraints on output buffers.
* **Avoid**: Channel reordering that breaks `(x,y) → thread` mapping; implicit vectorization on `x` with interleaved strides.

#### Thread-Level Constraints
On the GPU, `gpu_tile(x, y, ...)` maps `x` and `y` to GPU threads. We cannot reorder c outside the thread bounds without breaking the parallelism.

Instead, the absolute priority is satisfying the stride constraints. If the GPU backend attempts to vectorize the `x` dimension natively and encounters a stride of 4, it will attempt an unsupported scatter/gather operation, resulting in a silent driver crash (SIGABRT).

The solution is to leave the channel loop implicit inside the GPU threads, relying purely on the stride constraints to guide the backend compiler (CUDA/Vulkan) to generate correct memory access patterns for the `RGBA` layout without crashing.

```cpp
// No reorder on GPU. Just strict stride constraints + tiling.
pipeline.gpu_tile(x, y, xo, yo, xi, yi, 16, 16);
```

### For Both Backends

* **Always enforce output stride constraints**: `dim(0).set_stride(4)` and `dim(2).set_stride(1)` to guarantee Chunky compatibility with downstream systems.
* **Preserve Alpha**: Ensure the Alpha channel (`c == 3`) bypasses all color math and operations.
* **Defer clamping**: Apply final `[0,1]` clamping only after conversion back to RGB to avoid perceptual distortion.

---

## ✅ Benefits of This Strategy

| Aspect | Benefit |
|--------|---------|
| **Performance** | Eliminates 2 memory-shuffling passes per frame; CPU vectorization and GPU memory coalescing are optimized |
| **Correctness** | Explicit stride constraints prevent runtime errors; Oklab adjustments avoid hue shifts in shadows/highlights |
| **Maintainability** | Clear separation of concerns: boundary policy (Chunky) vs. internal optimization (Planar/Chunky) |
| **Extensibility** | Pattern ready for additional backends (e.g., CUDA, Vulkan) with their own layout strategies |
| **Portability** | Same conceptual flow works across Windows, macOS, Linux, and future mobile platforms |
