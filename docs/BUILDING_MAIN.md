### `docs/BUILDING_MAIN.md`
*The main entry point for build instructions.*


# 🚀 Build Guide - CaptureMoment

Welcome! This guide will help you build CaptureMoment from source.

Due to complex dependencies (specifically Halide and LLVM), instructions are separated by operating system to ensure the best performance and avoid build issues.

## 🧰 Global Prerequisites

Before selecting your platform, ensure you have these tools:

1.  **Git**: To clone the repository.
2.  **CMake 3.21+**: Our build system.
3.  **C++23 Compiler**:
    * **Windows**: MSVC 2022 or MinGW (GCC 13+).
    * **Linux**: GCC 13+ or Clang 16+.
    * **macOS**: Apple Clang 16+ (Xcode 15+).
4.  **Vcpkg** (Highly Recommended for Windows): To manage OpenImageIO, Halide, etc installations automatically.

---
## ⚙️ Global Build Configuration

### CMake Build
|  Component | Default Status |     CMake Variable (Alias)    |                  Description                  |
|:----------:|:--------------:|:-----------------------------:|:---------------------------------------------:|
| Desktop UI | OFF            | BUILD_DESKTOP_UI (desktop_ui) | Builds the Qt Quick/QML desktop application.  |
| Mobile UI  | OFF            | BUILD_MOBILE_UI (mobile_ui)   | Builds the mobile application (future phase). |
| Tests      | OFF            | BUILD_TESTS (tests)           | Builds unit and integration tests.            |
| Benchmarks | OFF            | BUILD_BENCHMARKS (benchmarks) | Builds performance benchmarks.                |

### Libraries

| Library     | Mandatory        | Link                                                     |
|-------------|:------------------:|----------------------------------------------------------|
| OpenImageIO | Yes              | https://github.com/AcademySoftwareFoundation/OpenImageIO |
| OpenColorIO | Yes              | https://github.com/AcademySoftwareFoundation/OpenColorIO |
| Halide      | Yes              | https://github.com/halide/Halide                         |
| spdlog      | Yes              | https://github.com/gabime/spdlog                         |
| Qt6         | No (only for UI) | https://doc.qt.io/qt-6/                                  |

---
## 📦 Vcpkg Configuration (General)

If you are using Vcpkg (recommended for Windows), the following points are essential:

VCPKG_ROOT : Ensure this environment variable points to your Vcpkg installation.

Binary Caching : To avoid recompiling LLVM (a huge Halide dependency) on every clean or triplet change, it is crucial to enable binary caching. Refer to the Windows guide for instructions.

---
## 🌍 Choose Your Platform

Click the link for your OS for detailed instructions:

* [🟦 **Windows**](guidelines/BUILDING_WINDOWS.md).
* [🐧 **Linux**](guidelines/BUILDING_LINUX.md) (Ubuntu, Fedora, Arch).
* [🍎 **macOS**](guidelines/BUILDING_MACOS.md) (Homebrew and Xcode).

## ⚡ Quick Start (Standard Machines)

If you have **Vcpkg** installed and configured, the universal command is:

```bash
git clone the repos
cd capturemoment
```
**Debug**
```bash
cmake --preset=debug-vcpkg
cmake --build build/debug-vcpkg
```
**Release**
```bash
cmake --preset=release-vcpkg
cmake --build build/release-vcpkg
```