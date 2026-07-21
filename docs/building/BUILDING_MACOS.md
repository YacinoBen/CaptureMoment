# macOS Building Guide
This guide covers building CaptureMoment on macOS. The recommended method uses an automated script to build core dependencies from source, ensuring you get the exact same versions and configuration as our Linux and CI pipelines, avoiding unpredictable Homebrew updates.

##  Setting up Dependencies (Recommended)

## 1. Run the automated setup script
This method uses an automated script that compiles Exiv2, OpenImageIO, and magic_enum from source to guarantee strict version consistency. (Note: The script uses Homebrew in the background to fetch the underlying base libraries required for the build).

From the root of the project, run:

```bash
./setup/setup-macos.sh
```
(Note: You can look inside this script if you prefer to run the installation steps manually).

## 🚀 Build Instructions
We use custom Homebrew presets that configure the necessary paths (CMAKE_PREFIX_PATH). Don't forget to enable the UI if you want!

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