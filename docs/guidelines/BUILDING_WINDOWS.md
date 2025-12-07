### `docs/build/BUILDING_WINDOWS.md`
*Detailed Windows guide addressing LLVM build times and disk space.*


# 🟦 Windows Build Guide
This guide is optimized to handle the challenges of building **Halide** and **LLVM** on Windows.

## ⚠️ Important: Disk Space & Build Time

Compiling dependencies (especially LLVM via Halide) is resource-intensive.
* **Standard Build** (Debug + Release): Requires ~80-100 GB. Can take 2h+.
* **Optimized Build** (Release only): Requires ~40 GB. ~40% faster.

You can build with vcpkg, it's better or you can use your own toolchains compilation

---
## 🛠️ Environment Setup with vcpkg

### 1. Install Vcpkg
```powershell
git clone [https://github.com/Microsoft/vcpkg.git](https://github.com/Microsoft/vcpkg.git) C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat
# Recommended:  Make VCPKG_ROOT permanent to system PATH
```

### 2. Enable Binary Caching (Crucial!)
To avoid recompiling LLVM every time you clean your project, enable local binary caching. Check if it's ON by default.

```PowerShell
# Create a permanent cache folder
mkdir C:\vcpkg_cache
# Set the variable (Make this permanent in Windows Environment Variables)
$env:VCPKG_BINARY_SOURCES = "clear;files,C:\vcpkg_cache"
```
---
## 🚀 Building the Project
We recommend using CMake Presets which automatically configure the correct generators and Vcpkg triplets. Don't forget to enable the UI with **-Ddesktop_ui=ON** if you want to compile also the ui
You can check [**The Main building**](../BUILDING_MAIN.md) to see all options

### Option A: Terminal - CMD
**Optimized Mode (Release only) :** (Uses the custom x64-windows-release triplet for time/space saving)

```PowerShell
cmake --preset release-vcpkg-msvc-desktop
cmake --build build/release-vcpkg-msvc-desktop
# or
cmake --preset release-vcpkg-msvc
cmake --build build/release-vcpkg-msvc
```
For Full Development (Debug):
=

```PowerShell
cmake --preset debug-vcpkg-msvc-desktop
cmake --build build/debug-vcpkg-msvc-desktop
# or
cmake --preset release-vcpkg-msvc
cmake --build build/release-vcpkg-msvc
```

### Option B: Qt Creator
**Optimized Mode (Release only) :** (Uses the custom x64-windows-release or x64-mingw-dynamic)

Open the root CMakeLists.txt with Qt Creator and choose the build you want. Espacially if you want to use the UI. choose msvc-qt or mingw-qt

### Option C: Visual Studio
Not configured at yet

---
## 🔍 Available Windows Presets
|     Preset Name     |       Generator       | Build Type |       VCPKG Triplet       |               Optimization / Notes              |
|:-------------------:|:---------------------:|:----------:|:-------------------------:|:-----------------------------------------------:|
| debug-vcpkg-msvc    | Auto-detected         | Debug      | x64-windows               | Standard Debug Build.                           |
| release-vcpkg-msvc  | Auto-detected         | Release    | x64-windows-release       | Optimized for Space/Time Savings (Recommended). |
| debug-vcpkg-mingw   | MinGW Makefiles       | Debug      | x64-mingw-dynamic         | For the MinGW toolchain.                        |
| release-vcpkg-mingw | MinGW Makefiles       | Release    | x64-mingw-dynamic-release | Optimized for MinGW (Time/Space Savings).       |
| debug-vs            | Visual Studio 17 2022 | Debug      | N/A                       | Without Vcpkg toolchain, uses default VS build. |
| release-vs          | Visual Studio 17 2022 | Release    | N/A                       | Without Vcpkg toolchain.                        |