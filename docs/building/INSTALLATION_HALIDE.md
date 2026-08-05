# 🏗️ Halide Installation
This guide provides clear instructions for obtaining Halide with support for various GPU backends (CUDA, OpenCL, Vulkan, DirectX12) as well as CPU-only configurations.

---

# GPU Backend supported

| Backend | Platform | Requirements |
|---------|----------|--------------|
| CUDA | NVIDIA GPUs | CUDA Toolkit installed, NVIDIA drivers |
| OpenCL | AMD/NVIDIA/Intel | OpenCL runtime (usually included with GPU drivers) |
| Vulkan | Cross-platform | Vulkan SDK and compatible driver |
| DirectX12 | Windows only | Windows 10/11, compatible GPU |

## How to Configure Halide

### 🥇 Method 1 : Pre-built Binary (Recommended & Fastest)
This is the easiest way to get a fully working Halide (with CPU + GPU support) in seconds without compiling LLVM.

#### Via PIP
```bash
pip3 install halide~=21.0.0
```

##### ⚠️ Important (Runtime Dependency)
After compiling, you must manually copy the Halide binary file (.dll, .so, or .dylib) from your Python site-packages directory into your final build folder so the executable can find it.

#### Via the official repos
[➡️ Download pre-build binary here](https://github.com/halide/Halide/releases)

When you compile the project with cmake, add this argument

```bash
-DHALIDE_DIR="path/to/halide/lib/cmake/Halide"
# Path to the cmake Halide
```

##### ⚠️ Important (Runtime Dependency):
CMake will successfully link the library during compilation, but your operating system needs help locating it at runtime. Once the build is complete, you must manually copy the shared library file (e.g., Halide.dll on Windows, .so on Linux, or .dylib on macOS) directly into your final build output directory, next to your executable.

* **Example on Windows**

```bash
cmake --preset release-vcpkg-msvc -DHALIDE_DIR="C:/project/halide/lib/cmake/Halide"
```

### 🥈 Method 2: Package Managers (CPU Only)

#### Ubuntu / Debian (APT)

```bash
sudo apt install libhalide-dev
```

[➡️ See here if the name package is invalid](https://launchpad.net/ubuntu/+source/halide/)
 
❌ **Note**: APT packages do not include GPU support.

#### macOS (Homebrew)

```bash
brew install halide
```
[➡️ More details](https://formulae.brew.sh/formula/halide)

❌ **Note**: Maybe packages do not include GPU support.

#### Windows (vcpkg)

```bash
vcpkg install halide:x64-windows
```

[➡️ More details](https://vcpkg.link/ports/halide)

You can also install GPU support as Feature Dependencies


### 🥉 Method 3: Build from Source (For Advanced Users / Full GPU Control)

🔧 **Building from source is the only reliable way to enable full GPU support**. Users requiring GPU acceleration should follow the official CMake build documentation to compile Halide with the desired backends enabled.

- **GitHub Repository**: https://github.com/halide/Halide
- **Official Build Guide**: https://halide-lang.org/docs/md_doc_2_building_halide_with_c_make.html

⚠️ Important: Disk Space & Build Time

Compiling dependencies (especially LLVM via Halide) is resource-intensive.
* **Standard Build** (Debug + Release): Requires ~80-100 GB. Can take 2h+.
* **Optimized Build** (Release only): Requires ~40 GB. ~40% faster.

#### Add to the project
* When you want to build the projet Capture Moment. You can just set the path by adding this argument when you start building at the final with cmake.

```bash
-DHALIDE_DIR="path/to/halide/lib/cmake/Halide"
# Path to the cmake Halide
```