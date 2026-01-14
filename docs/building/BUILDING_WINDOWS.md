# 🟦 Windows Build Guide
This guide is optimized to handle the challenges of building **Halide** and **LLVM** on Windows.

## ⚠️ Important: Disk Space & Build Time

Compiling dependencies (especially LLVM via Halide) is resource-intensive.
* **Standard Build** (Debug + Release): Requires ~80-100 GB. Can take 2h+.
* **Optimized Build** (Release only): Requires ~40 GB. ~40% faster.

You can build with vcpkg, it's better or you can use your own toolchains compilation

---

## Build Halide
It's the most part it will take the time for building. It's better to choose pip method if you don't any binary installed.

### vcpkg
You can build it from vcpkg, it can be take more than 3 hours for a specific building. and it install a old version

```powershell
vcpkg install halide
```
### pip
The most easier, and it willl install the last version
```powershell
pip install halide
```
### Source
Just follow how to build : https://github.com/halide/Halide

### Important
If you want to use your halide build, you need to use this argument "-DHALIDE_DIR=/path/to/halide(cmake)"

Otherwise if you have installed halide with pip or you have already set the path in your variable env, you don't need to add the argument.

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

It's important to put the path of your folder halide (where we find .cmake) builded with pip or vcpkg or another method to. Use this argument -DHALIDE_DIR=/path/to/halide(cmake)

Or you can just add a path env for your HALIDE_DIR

### Option A: Only the Core
**Optimized Mode (Release only) :** (Uses the custom x64-windows-release triplet for time/space saving)

```PowerShell
cmake --preset release-vcpkg-msvc -DHALIDE_DIR=/path/to/halide(cmake)
cmake --build build/release-vcpkg-msvc
# or
cmake --preset release-vcpkg-mingw -DHALIDE_DIR=/path/to/halide(cmake)
cmake --build build/release-vcpkg-mingw

#or configure your path env
cmake --preset release-vcpkg-mingw
cmake --build build/release-vcpkg-mingw
```
For Full Development (Debug):
=

```PowerShell
cmake --preset debug-vcpkg-msvc -DHALIDE_DIR=/path/to/halide(cmake)
cmake --build build/debug-vcpkg-msvc
```

### Option B: With Qt Desktop

#### If you have Qt and you don't want to recompile again

```PowerShell
cmake --preset release-vcpkg-msvc -DHALIDE_DIR=/path/to/halide(cmake) -Ddesktop-ui="ON"
cmake --build build/release-vcpkg-msvc
# or
cmake --preset release-vcpkg-mingw -DHALIDE_DIR=/path/to/halide(cmake) -Ddesktop-ui="ON"
cmake --build build/release-vcpkg-mingw
```

#### If you don't have Qt
```PowerShell
cmake --preset release-vcpkg-msvc-desktop -DHALIDE_DIR=/path/to/halide(cmake)
cmake --build build/release-vcpkg-msvc-desktop
# or
cmake --preset release-vcpkg-mingw-desktop -DHALIDE_DIR=/path/to/halide(cmake)
cmake --build build/release-vcpkg-mingw
```

### Option C: IDE
Don't forget to use -DHALIDE_DIR=/path/to/halide(cmake) of your IDE build to add the path of Halide
#### Qt Creator
Open the root CMakeLists.txt with Qt Creator and choose the build vcpkg core msvc or mingw (with no desktop). And when you are in the pannel of the configuration, activate BUILD_UIBUILD_DESKTOP_UI to ON

#### Visual Studio
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