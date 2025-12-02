# 🏗️ Architecture & Technical Stack - CaptureMoment

This document outlines the technical architecture, dependencies, and design principles of the project.

## 🎯 Core Principles

Capture Moment is built on three pillars:
1.  **Non-Destructive Workflow**: The source file (RAW/TIFF) is never modified. Adjustments are stored as a metadata "recipe".
2.  **High-Performance Pipeline**: Uses **Halide** to fuse all processing operations into a single computation pass, avoiding intermediate memory reads/writes.
3.  **Modularity**: Strict separation between I/O (Loading), Compute (Math), and Display (UI).

---

## 📦 Technical Stack

### 1. Data Management & I/O (Source Manager)
* **Library:** [OpenImageIO (OIIO)](https://openimageio.readthedocs.io/)
* **Role:**
    * **Input/Output:** Reads RAW files (via integrated LibRaw), TIFF, EXR, JPG, etc..
    * **Image Cache:** Handles tiling and lazy loading to manage 50MP+ files with limited RAM.
    * **Metadata:** Manages EXIF, XMP, and IPTC data.
    * **Color:** Bridges with OpenColorIO.

### 2. Compute Engine (Pipeline Engine)
* **Library:** [Halide](https://halide-lang.org/)
* **Role:**
    * **Processing:** Executes algorithms (Exposure, Contrast, Sharpening, Denoise).
    * **AOT Compilation:** Filters are compiled Ahead-Of-Time into optimized machine code (AVX2/NEON/GPU).
    * **Optimization:** Automatic vectorization, parallelism, and loop fusion.

### 3. User Interface & Display
* **Framework:** [Qt 6 (Quick/QML)](https://doc.qt.io/)
* **Role:**
    * **UI:** Modern, fluid, declarative interface.
    * **Rendering:** Uses **QRhi** (Qt Rendering Hardware Interface) to display Halide buffers directly on the GPU (Vulkan/Metal/D3D12).

### 4. Dependency Management
* **Tool:** [Vcpkg](https://vcpkg.io/) (Manifest Mode)
* **Role:** Ensures all libraries (OIIO, OCIO, Halide, Qt) are compiled with the same ABI across all platforms.


## 📚 Additional Resources

- **CMake Documentation:** [cmake.org/documentation](https://cmake.org/documentation/)
- **Qt Documentation:** [doc.qt.io](https://doc.qt.io/)
- **OpenImageIO:** [openimageio.readthedocs.io](https://openimageio.readthedocs.io/)
- **vcpkg:** [github.com/Microsoft/vcpkg](https://github.com/Microsoft/vcpkg)
