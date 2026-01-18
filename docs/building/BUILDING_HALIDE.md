# 🏗️ Building Halide 21 with Full GPU Support

## Overview

This guide provides clear instructions for obtaining Halide 21.0.0 or newer with support for various GPU backends (CUDA, OpenCL, Vulkan, DirectX12) as well as CPU-only configurations.

⚠️ **Important**: The official pre-built binaries do not include GPU support by default. To use GPU acceleration, you must either:
- Use a package manager that explicitly enables GPU features, or
- Build Halide from source with the appropriate flags (see official documentation).

⚠️ Important: Disk Space & Build Time

Compiling dependencies (especially LLVM via Halide) is resource-intensive.
* **Standard Build** (Debug + Release): Requires ~80-100 GB. Can take 2h+.
* **Optimized Build** (Release only): Requires ~40 GB. ~40% faster.
---

## System Requirements

### LLVM
- **Required**: LLVM 21.1.1 or higher (LLVM 21.1.0 has a critical bug in the NVPTX backend and must be avoided)

### GPU Backend Requirements

| Backend | Platform | Requirements |
|---------|----------|--------------|
| CUDA | NVIDIA GPUs | CUDA Toolkit installed, NVIDIA drivers |
| OpenCL | AMD/NVIDIA/Intel | OpenCL runtime (usually included with GPU drivers) |
| Vulkan | Cross-platform | Vulkan SDK and compatible driver |
| DirectX12 | Windows only | Windows 10/11, compatible GPU |

### CPU Support
- Always available; no additional dependencies required.
- Recommended for development or when GPU drivers are unavailable.

## Installation Method From Release (ONLY CPU)
* Download a release from here : https://github.com/halide/Halide/releases

* And in your terminal execute :

```bash
cmake --preset release-vcpkg-msvc -Ddesktop_ui="on" -DHALIDE_DIR="pathto/CaptureMoment/halide/lib/cmake/Halide"
# Path to the cmake Halide
```
You can see more details about the excetion in the corresponding files (BUILDING_xxx.md)

## Installation Methods From Dependecies Managers

### Ubuntu / Debian (APT)

**More details:** https://launchpad.net/ubuntu/+source/halide/

❌ **Note**: APT packages do not include GPU support.

### macOS (Homebrew)

```bash
brew install halide
```
**More details:** https://formulae.brew.sh/formula/halide
❌ **Note**: Maybe packages do not include GPU support.

### Windows (vcpkg)

**More details:** https://vcpkg.link/ports/halide

You can also install GPU support as Feature Dependencies

## Source Code & Official Build Documentation

- **GitHub Repository**: https://github.com/halide/Halide
- **Official Build Guide**: https://halide-lang.org/docs/md_doc_2_building_halide_with_c_make.html

🔧 **Building from source is the only reliable way to enable full GPU support** (especially CUDA). Users requiring GPU acceleration should follow the official CMake build documentation to compile Halide with the desired backends enabled.

## Version Requirement

- **Minimum Version**: Halide 21.0.0
- Older versions lack critical GPU scheduling improvements and may not support modern hardware features.

## Runtime Backend Detection

Your application should always check for available backends at runtime:

```cpp
bool hasGPUSupport() {
    try {
        auto gpu = Halide::get_device_api(Halide::DeviceAPI::Metal);
        return gpu != nullptr;
    } catch (...) {
        return false;
    }
}
```

**Always provide a CPU fallback for systems without GPU drivers or unsupported hardware.**
