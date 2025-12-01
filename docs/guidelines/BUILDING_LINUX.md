### `docs/build/BUILDING_LINUX.md`
*Detailed Linux guide addressing LLVM build times and disk space.*


# 🐧 Linux Building Guide
On Linux, you can choose between using system packages (faster) or Vcpkg (isolated environment).

## 📦 Method 1: System Packages (Recommended)

This is the fastest method as it uses pre-compiled binaries via your distribution's package manager, thus avoiding the long LLVM compilation time.

**Ubuntu 24.04+ / Debian 12+**
```powershell
sudo apt update
sudo apt install -y \
    build-essential cmake ninja-build git \
    qt6-base-dev qt6-declarative-dev qt6-tools-dev \
    libopenimageio-dev libhalide-dev
```

**Fedora 40+**
```powershell
sudo dnf install -y \
    gcc-c++ cmake ninja-build git \
    qt6-qtbase-devel qt6-qtdeclarative-devel \
    OpenImageIO-devel halide-devel
```

**Build Commands**

Since dependencies are installed system-wide, we use generic presets. Don't forget to enable the UI if you wanna to build it!

```powershell
# Configure in Debug and enable the UI
cmake --preset debug -Ddesktop_ui=ON
# Compile using all available cores
cmake --build build/debug -j$(nproc)
```


## 📦Method 2: Vcpkg (Isolated)

Use this method if you need specific library versions or want a development environment identical to Windows.

* Follow the Vcpkg installation steps.
* Configure using the generic preset, ensuring the VCPKG_ROOT environment variable is defined.

```powershell
# Example using the debug preset (Vcpkg toolchain is automatically detected if VCPKG_ROOT is set)
cmake --preset debug -Ddesktop_ui=ON
cmake --build build/debug -j$(nproc)
```

---
## 🔍 Available Linux Presets

|  Preset Name |    Generator   | Build Type |                       Notes                      |
|:------------:|:--------------:|:----------:|:------------------------------------------------:|
| default      | Auto-detect    | Debug      | Auto-detects the generator (Ninja or Makefiles). |
| debug        | Auto-detect    | Debug      | Base preset for development.                     |
| release      | Auto-detect    | Release    | Optimized build.                                 |
| debug-ninja  | Ninja          | Debug      | Ninja generator for fast build (recommended).    |
| release-ninja| Ninja          | Release    | Ninja generator for fast build (recommended).    |
| release-make | Unix Makefiles | Release    | Classic Makefiles generator.                     |
| debug-make   | Unix Makefiles | Debug      | Classic Makefiles generator.                     |