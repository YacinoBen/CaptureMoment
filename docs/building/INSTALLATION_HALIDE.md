# 🏗️ Halide Installation

This guide provides clear instructions for obtaining Halide with support for various GPU backends (CUDA, OpenCL, Vulkan, DirectX12) as well as CPU-only configurations.

⚠️ **Important**: The official pre-built binaries do not include GPU support by default. To use GPU acceleration, you must either:
- Use a package manager that explicitly enables GPU features
- Build Halide from source with the appropriate flags (see official documentation).

⚠️ Important: Disk Space & Build Time

Compiling dependencies (especially LLVM via Halide) is resource-intensive.
* **Standard Build** (Debug + Release): Requires ~80-100 GB. Can take 2h+.
* **Optimized Build** (Release only): Requires ~40 GB. ~40% faster.
---

## GPU Backend Requirements

| Backend | Platform | Requirements |
|---------|----------|--------------|
| CUDA | NVIDIA GPUs | CUDA Toolkit installed, NVIDIA drivers |
| OpenCL | AMD/NVIDIA/Intel | OpenCL runtime (usually included with GPU drivers) |
| Vulkan | Cross-platform | Vulkan SDK and compatible driver |
| DirectX12 | Windows only | Windows 10/11, compatible GPU |


## Automatic installation Method With ONLY CPU

### Installation Method With PIP

```bash
pip3 install halide
```

### Installation Methods From Dependecies Managers

#### Ubuntu / Debian (APT)

**More details:** https://launchpad.net/ubuntu/+source/halide/

❌ **Note**: APT packages do not include GPU support.

#### macOS (Homebrew)

```bash
brew install halide
```
**More details:** https://formulae.brew.sh/formula/halide

❌ **Note**: Maybe packages do not include GPU support.

#### Windows (vcpkg)

```bash
vcpkg install halide:x64-windows
```

**More details:** https://vcpkg.link/ports/halide

You can also install GPU support as Feature Dependencies



## Manual installation 

### From Release repos (ONLY CPY)
* Download a release from here : https://github.com/halide/Halide/releases

#### Add to the project
* When you want to build the projet Capture Moment. You can just set the path by adding this argument when you start building at the final with cmake.

```bash
-DHALIDE_DIR="pathtohalide/lib/cmake/Halide"
# Path to the cmake Halide
```


## Source Code & Official Build Documentation

- **GitHub Repository**: https://github.com/halide/Halide
- **Official Build Guide**: https://halide-lang.org/docs/md_doc_2_building_halide_with_c_make.html

🔧 **Building from source is the only reliable way to enable full GPU support** (especially CUDA). Users requiring GPU acceleration should follow the official CMake build documentation to compile Halide with the desired backends enabled.
