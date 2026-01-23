### `docs/BUILDING_MAIN.md`
*The main entry point for build instructions.*


# 🚀 Build Guide - CaptureMoment

Welcome! This guide will help you build CaptureMoment from source.

Due to complex dependencies (specifically Halide and LLVM), instructions are separated by operating system to ensure the best performance and avoid build issues.

## 🧰 Global Prerequisites

Before selecting your platform, ensure you have these tools:

1.  **CMake 3.27+**
2.  **C++23 Compiler**:
    * **Windows**: MSVC 2022 or MinGW (GCC 13+).
    * **Linux**: GCC 13+ or Clang 16+.
    * **macOS**: Apple Clang 16+ (Xcode 15+).

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

| Library     | Version      | Link                                                     |
|-------------|:------------------:|----------------------------------------------------------|
| OpenImageIO | 3.1.8.0             | https://github.com/AcademySoftwareFoundation/OpenImageIO |
| Halide      | 21.x +           | https://github.com/halide/Halide                         |
| spdlog      | 1.16.0      | https://github.com/gabime/spdlog                         |
| Exiv2         | 0.28.7| https://github.com/Exiv2/exiv2  
| Magicmagic_enum          |0.9.7| https://github.com/Neargye/magic_enum/
|  Qt6           | 6.10| https://doc.qt.io/qt-6/ 

---
## 📦 Vcpkg Configuration (General)

If you are using Vcpkg (recommended for Windows), the following points are essential:

VCPKG_ROOT : Ensure this environment variable points to your Vcpkg installation.

Binary Caching : To avoid recompiling LLVM (a huge Halide dependency) on every clean or triplet change, it is crucial to enable binary caching. Refer to the Windows guide for instructions.

---
## 🌍 Choose Your Platform

Click the link for your OS for detailed instructions:

* [🟦 **Windows**](./guidelines/BUILDING_WINDOWS.md).
* [🐧 **Linux**](./guidelines/BUILDING_LINUX.md) (Ubuntu, Fedora, Arch).
* [🍎 **macOS**](./guidelines/BUILDING_MACOS.md) (Homebrew and Xcode).

## Understood the core 

* [**The Core**](../architecture/CORE_DESIGN.md) (You can see the design patterns used for the core).
