# 🏗️ Build Guide for CaptureMoment

This comprehensive guide explains how to **build CaptureMoment from source** on **Windows**, **macOS**, and **Linux** using various build systems and package managers.

---

## 📋 Table of Contents

- [🏗️ Build Guide for CaptureMoment](#️-build-guide-for-capturemoment)
  - [📋 Table of Contents](#-table-of-contents)
  - [🧰 Prerequisites](#-prerequisites)
    - [Compiler Compatibility](#compiler-compatibility)
  - [🚀 Quick Start](#-quick-start)
    - [Linux (Ubuntu/Debian)](#linux-ubuntudebian)
    - [macOS (Homebrew)](#macos-homebrew)
    - [Windows (vcpkg)](#windows-vcpkg)
  - [📦 Dependencies](#-dependencies)
    - [Required Libraries](#required-libraries)
    - [Optional Libraries (Future Phases)](#optional-libraries-future-phases)
  - [🔧 Build Methods](#-build-methods)
    - [Method 1: System Package Manager (Recommended for Development)](#method-1-system-package-manager-recommended-for-development)
    - [Observation](#observation)
      - [Linux (Ubuntu 24.04+)](#linux-ubuntu-2404)
      - [Linux (Fedora 40+)](#linux-fedora-40)
      - [Linux (Arch)](#linux-arch)
      - [MacOS (Homebrew recommended)](#macos-homebrew-recommended)
      - [Windows (vcpkg recommended)](#windows-vcpkg-recommended)
  - [Build the project](#build-the-project)
    - [Method 3: Manual Installation](#method-3-manual-installation)
      - [Manual OpenImageIO Compilation Example](#manual-openimageio-compilation-example)
  - [🎛️ CMake Presets](#️-cmake-presets)
    - [Available Presets](#available-presets)
    - [Using Presets](#using-presets)
      - [List Available Presets](#list-available-presets)
      - [Configure with Preset](#configure-with-preset)
      - [Build with Preset](#build-with-preset)
  - [⚙️ Build Options](#️-build-options)
    - [Available Options](#available-options)
    - [Aliases (Shorter Syntax)](#aliases-shorter-syntax)
    - [Examples](#examples)
  - [🛠️ Troubleshooting](#️-troubleshooting)
    - [Common Issues](#common-issues)
      - [Issue 1: "OpenImageIO not found"](#issue-1-openimageio-not-found)
  - [📚 Additional Resources](#-additional-resources)
  - [📝 Summary: Common Build Commands](#-summary-common-build-commands)

---

## 🧰 Prerequisites

Before building, install the following tools:

| Component | Minimum Version | Download / Installation |
|-----------|----------------|------------------------|
| **C++ Compiler** | C++23 support | GCC 13+, Clang 16+, MSVC 2022+ |
| **CMake** | 3.21+ | [cmake.org](https://cmake.org/download/) |
| **Git** | Latest | [git-scm.com](https://git-scm.com/) |
| **Build System** | Any | Ninja (recommended), Make, Visual Studio, MinGW |

### Compiler Compatibility

| Platform | Recommended Compiler |
|----------|---------------------|
| **Windows** | MSVC 2022 (17.8+) or MinGW-w64 GCC 13+ |
| **macOS** | Apple Clang 16+ (Xcode 15+) |
| **Linux** | GCC 13+ or Clang 16+ |

---

## 🚀 Quick Start

### Linux (Ubuntu/Debian)
```bash
# 1. Install dependencies
sudo apt update
sudo apt install -y \
    build-essential cmake ninja-build git \
    qt6-base-dev qt6-declarative-dev qt6-tools-dev \
    libopenimageio-dev

# 2. Clone repository
git clone https://github.com/your-org/capturemoment.git
cd capturemoment

# 3. Configure with preset
cmake --preset=debug -Ddesktop_ui=ON

# 4. Build
cmake --build build/debug

# 5. Run
./build/debug/ui/desktop/capturemoment_desktop
```

### macOS (Homebrew)
```bash
# 1. Install Homebrew (if not installed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 2. Install dependencies
brew install cmake ninja git qt@6 openimageio

# 3. Clone repository
git clone https://github.com/your-org/capturemoment.git
cd capturemoment

# 4. Configure with Homebrew preset
cmake --preset=debug-homebrew -Ddesktop_ui=ON

# 5. Build
cmake --build build/debug-homebrew

# 6. Run
./build/debug-homebrew/ui/desktop/capturemoment_desktop.app/Contents/MacOS/capturemoment_desktop
```

### Windows (vcpkg)
```powershell
# 1. Install vcpkg (if not installed)
git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat

# 3. Set environment variable  vcpkg
You can see follow a tutorial

# 4. Clone repository
git clone https://github.com/your-org/capturemoment.git
cd capturemoment

# 2. Install dependencies in manifest mode
# Run the terminal on your root project and execute the following commands
.\vcpkg install

# 5. Configure with vcpkg preset
cmake --preset=debug-vcpkg -Ddesktop_ui=ON

# 6. Build
cmake --build build/debug-vcpkg

# 7. Run
.\build\debug-vcpkg\ui\desktop\Debug\capturemoment_desktop.exe
```

---

## 📦 Dependencies

### Required Libraries

| Library | Purpose | Phase |
|---------|---------|-------|
| **Qt6 Base** | Core framework (QObject, signals/slots) | 1 |
| **Qt6 Quick** | QML runtime and UI components | 1 |
| **OpenImageIO** | Image I/O, RAW decoding, caching | 1 |

### Optional Libraries (Future Phases)

| Library | Purpose | Phase |
|---------|---------|-------|
| **Halide** | High-performance image processing | 2 |
| **OpenColorIO** | Professional color management | 2 |
| **SQLite** | Database for cataloging | 2 |

---

## 🔧 Build Methods

### Method 1: System Package Manager (Recommended for Development)

**Pros:** ✅ Simple, fast, well-tested  
**Cons:** ❌ May have outdated versions

### Observation
If you don't want to compile the UI. You can skip the Qt Installation

#### Linux (Ubuntu 24.04+)
```bash
# Install all dependencies
sudo apt install -y \
    build-essential cmake ninja-build git \
    qt6-base-dev qt6-declarative-dev qt6-qmllint \
    libopenimageio-dev

# Configure and build
cmake --preset=debug -Ddesktop_ui=ON
cmake --build build/debug -j$(nproc)
```

#### Linux (Fedora 40+)
```bash
# Install dependencies
sudo dnf install -y \
    gcc-c++ cmake ninja-build git \
    qt6-qtbase-devel qt6-qtdeclarative-devel \
    OpenImageIO-devel

# Build
cmake --preset=debug -Ddesktop_ui=ON
cmake --build build/debug -j$(nproc)
```

#### Linux (Arch)
```bash
# Install dependencies
sudo pacman -S --needed \
    base-devel cmake ninja git \
    qt6-base qt6-declarative \
    openimageio

# Build
cmake --preset=debug -Ddesktop_ui=ON
cmake --build build/debug -j$(nproc)
```

#### MacOS (Homebrew recommended)
```bash
# Install dependencies
brew install cmake ninja git qt@6 openimageio

# Qt path might need to be set
export CMAKE_PREFIX_PATH="$(brew --prefix qt@6)"

# Build
cmake --preset=debug-homebrew -Ddesktop_ui=ON
cmake --build build/debug-homebrew -j$(sysctl -n hw.ncpu)
```

#### Windows (vcpkg recommended) 
```bash
# Install for Windows (x64)
./vcpkg install openimageio:x64-windows qt6-base:x64-windows qt6-declarative:x64-windows

```
## Build the project
```bash
# Clone project
git clone https://github.com/your-org/capturemoment.git
cd capturemoment

# Configure with vcpkg preset as example
cmake --preset=debug-vcpkg -Ddesktop_ui=ON

# Build
cmake --build build/debug-vcpkg --config Debug
```

---

### Method 3: Manual Installation

**Pros:** ✅ Full control, latest features  
**Cons:** ❌ Complex, time-consuming

#### Manual OpenImageIO Compilation Example
```bash
# Clone and build OIIO
git clone https://github.com/AcademySoftwareFoundation/OpenImageIO.git
cd OpenImageIO
mkdir build && cd build

cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/usr/local \
      ..

cmake --build . -j$(nproc)
sudo cmake --install .
```

---

## 🎛️ CMake Presets

CaptureMoment provides multiple **CMake Presets** for different platforms and build systems.

### Available Presets

| Preset Name | Generator | Platform | Description |
|-------------|-----------|----------|-------------|
| `debug` | Auto-detect | All | Debug build (default) |
| `release` | Auto-detect | All | Optimized release build |
| `relwithdebinfo` | Auto-detect | All | Release with debug symbols |
| `debug-ninja` | Ninja | All | Debug with Ninja (fast) |
| `release-ninja` | Ninja | All | Release with Ninja |
| `debug-make` | Unix Makefiles | Linux/macOS | Debug with Make |
| `release-make` | Unix Makefiles | Linux/macOS | Release with Make |
| `debug-mingw` | MinGW Makefiles | Windows | Debug with MinGW |
| `release-mingw` | MinGW Makefiles | Windows | Release with MinGW |
| `debug-vs` | Visual Studio 2022 | Windows | Debug with Visual Studio |
| `release-vs` | Visual Studio 2022 | Windows | Release with Visual Studio |
| `debug-vcpkg` | Auto-detect | All | Debug using vcpkg dependencies |
| `release-vcpkg` | Auto-detect | All | Release using vcpkg |
| `debug-homebrew` | Ninja | macOS | Debug using Homebrew |
| `release-homebrew` | Ninja | macOS | Release using Homebrew |

### Using Presets

#### List Available Presets
```bash
cmake --list-presets
```

#### Configure with Preset
```bash
# Syntax
cmake --preset=<preset-name> [options]

# Examples
cmake --preset=debug -Ddesktop_ui=ON
cmake --preset=release-ninja -Ddesktop_ui=ON -Dtests=ON
cmake --preset=debug-vcpkg -Ddesktop_ui=ON
```

#### Build with Preset
```bash
# Build using preset name
cmake --build --preset=debug

# Or specify build directory
cmake --build build/debug
```

---

## ⚙️ Build Options

Control what gets built using CMake options:

### Available Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_DESKTOP_UI` | OFF | Build desktop Qt Quick application |
| `BUILD_MOBILE_UI` | OFF | Build mobile UI (future) |
| `BUILD_TESTS` | OFF | Build unit tests |
| `BUILD_BENCHMARKS` | OFF | Build performance benchmarks |

### Aliases (Shorter Syntax)

| Alias | Equivalent |
|-------|-----------|
| `desktop_ui=ON` | `BUILD_DESKTOP_UI=ON` |
| `mobile_ui=ON` | `BUILD_MOBILE_UI=ON` |
| `tests=ON` | `BUILD_TESTS=ON` |
| `benchmarks=ON` | `BUILD_BENCHMARKS=ON` |

### Examples
```bash
# Build desktop UI only
cmake --preset=debug -Ddesktop_ui=ON

# Build desktop UI + tests
cmake --preset=debug -Ddesktop_ui=ON -Dtests=ON

# Build everything
cmake --preset=release -Ddesktop_ui=ON -Dmobile_ui=ON -Dtests=ON -Dbenchmarks=ON

# Equivalent with full option names
cmake --preset=release \
    -DBUILD_DESKTOP_UI=ON \
    -DBUILD_MOBILE_UI=ON \
    -DBUILD_TESTS=ON \
    -DBUILD_BENCHMARKS=ON
```

---

## 🛠️ Troubleshooting

### Common Issues

#### Issue 1: "OpenImageIO not found"

**Symptoms:**
```
CMake Error: Could not find OpenImageIO
```

**Solutions:**

**Linux:**
```bash
sudo apt install libopenimageio-dev
```

**macOS:**
```bash
brew install openimageio
```

**Windows (vcpkg):**
```powershell
vcpkg install openimageio:x64-windows
```

**Manual Path (all platforms):**
```bash
cmake --preset=debug \
    -DCMAKE_PREFIX_PATH="/path/to/openimageio/install" \
    -Ddesktop_ui=ON
```
---

## 📚 Additional Resources

- **CMake Documentation:** [cmake.org/documentation](https://cmake.org/documentation/)
- **Qt Documentation:** [doc.qt.io](https://doc.qt.io/)
- **OpenImageIO:** [openimageio.readthedocs.io](https://openimageio.readthedocs.io/)
- **vcpkg:** [github.com/Microsoft/vcpkg](https://github.com/Microsoft/vcpkg)

---

## 📝 Summary: Common Build Commands

| Scenario | Command |
|----------|---------|
| **Linux (apt)** | `cmake --preset=debug -Ddesktop_ui=ON && cmake --build build/debug` |
| **macOS (Homebrew)** | `cmake --preset=debug-homebrew -Ddesktop_ui=ON && cmake --build build/debug-homebrew` |
| **Windows (vcpkg)** | `cmake --preset=debug-vcpkg -Ddesktop_ui=ON && cmake --build build/debug-vcpkg` |
| **Release build** | `cmake --preset=release -Ddesktop_ui=ON && cmake --build build/release` |
| **With tests** | `cmake --preset=debug -Ddesktop_ui=ON -Dtests=ON && cmake --build build/debug` |
| **Run tests** | `ctest --test-dir build/debug --output-on-failure` |
| **Clean build** | `rm -rf build && cmake --preset=debug -Ddesktop_ui=ON` |

---

**Happy Building! 🎉**

If you encounter issues not covered here, please open an issue on GitHub.