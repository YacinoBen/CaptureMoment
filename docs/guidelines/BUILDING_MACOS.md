### `docs/build/BUILDING_MACOS.md`
*For macOS, we recommend using Homebrew to manage dependencies (Qt, OpenImageIO, Halide).*


# macOS Building Guide
For macOS, we recommend using Homebrew to manage dependencies (Qt, OpenImageIO, Halide).

## Prerequisites
* Xcode
* Homebrew

## 📦Installing Dependencies

```powershell
brew update
brew install cmake ninja git
brew install qt@6
brew install openimageio
brew install halide

```


## 🚀 Build Instructions
We use custom Homebrew presets that configure the necessary paths (CMAKE_PREFIX_PATH). Don't forget to enable the UI!

**Compilation**
```powershell
# Use the optimized Homebrew preset and enable the Desktop UI
cmake --preset debug-homebrew -Ddesktop_ui=ON
```
**Configuration**
```powershell
# Compile using all available cores
cmake --build build/debug-homebrew -j$(sysctl -n hw.ncpu)
```
---
## 🔍 Available macOS Presets (Homebrew)

|    Preset Name   |  Generator  | Build Type |     CMAKE_PREFIX_PATH    |                               Notes                              |
|:----------------:|:-----------:|:----------:|:------------------------:|:----------------------------------------------------------------:|
| debug-homebrew   | Ninja       | Debug      | /opt/homebrew;/usr/local | Recommended. Uses Ninja for a fast build.                        |
| release-homebrew | Ninja       | Release    | /opt/homebrew;/usr/local | Optimized final build.                                           |
| debug            | Auto-detect | Debug      | N/A                      | Generic preset (may require manually defining dependency paths). |
| release-ninja    | Ninja       | Release    | N/A                      | Alternative for the optimized build.                             |
| debug-ninja      | Ninja       | Debug      | N/A                      | N/A                                                              |                           |