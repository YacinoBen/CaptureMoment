# 🐧 Linux Building Guide
This guide covers building CaptureMoment on Linux. You can choose between using system packages (faster) or building dependencies from source (more control, potentially newer versions).

## Qt-UI
If you want to develop the UI with Qt, you can directly download the actual version (e.g., 6.9.3) from the official website.

## 📦 Building with System Packages (Recommended)

This is the fastest method as it uses pre-compiled binaries via your distribution's package manager, thus avoiding the long compilation times of dependencies like LLVM (for Halide) or Boost (for Exiv2).

### Ubuntu 25.04 / Debian Testing (or newer versions with required packages)

Install the necessary build tools and dependencies using the package manager:

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build

# Logging
sudo apt install -y libspdlog-dev

# Image I/O and Processing
sudo apt install -y libopenimageio-dev # Includes dependencies like libtiff-dev, libjpeg-dev, etc.

# Metadata Handling
sudo apt install -y libexiv2-dev # For XMP metadata
sudo apt install -y libmagicenum-dev # For safe enum-to-string conversion

# Image Processing Backend (Choose one based on availability/version)
# Check available versions: apt search halide
sudo apt install -y libhalide21-dev  # Preferably the latest supported version (e.g., 21.x)
# sudo apt install -y libhalide19-dev # Fallback if 21 is not available

# Potentially needed system libraries
sudo apt install -y libcurl4-openssl-dev # For network features in Exiv2 (if enabled)
sudo apt install -y libxkbcommon-dev     # For Qt Wayland support (if applicable)
```

## 📦 Manual Build of Dependencies (Advanced/Custom)

Use this method if system packages are unavailable, too old, or if you need specific configurations not provided by your distribution. This involves downloading, configuring, compiling, and installing libraries individually.

Make sure you have : ninja, cmake 

### Install the libraries
```powershell
sudo apt install -y libspdlog-dev 

# If you can compile cmake with halide installed with pip ok, otherwise delete it and
# Install libhalide-dev, generally in ubuntu 24 it's 17-1, but in Ubuntu 25 maybe you need to change the version.
pip uninstall halide -y halide || true

# Execute the following if you don't have halide or pip halide can't 
# compile the project with cmake
sudo apt install -y libhalide17-1 libhalide17-1-dev python3-halide

# Maybe we need it
sudo apt install -y libcurl4-openssl-dev
sudo apt install -y libxkbcommon-dev
```

### Build OpenImageIO
```powershell
sudo apt install -y \
libjpeg-dev \
libfreetype-dev \
libpng-dev \
libuhd-dev \
libraw-dev \
libopencv-core-dev libopencv-imgproc-dev \
libjxl-dev \
libheif-dev \
libpystring-dev \
libtiff-dev \
libimath-dev \
libopenexr-dev \
libopencolorio-dev \
zlib1g-dev \
libjpeg-turbo8-dev
```

Compilation :

```powershell
 git clone https://github.com/AcademySoftwareFoundation/OpenImageIO.git /tmp/oiio-src
cd /tmp/oiio-src
git checkout v3.1.8.0
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=${{ matrix.build-type }} \
-DCMAKE_INSTALL_PREFIX=/opt/oiio \
-DOpenImageIO_BUILD_MISSING_DEPS=required \
-DSTOP_ON_WARNING=0 \
-DOIIO_BUILD_TOOLS=OFF \
-DOIIO_BUILD_TESTS=OFF \
-DUSE_PYTHON=OFF \
..

cmake --build . --parallel $(nproc)
sudo cmake --install .
```

### magic-enum
Magic enum is header-only, so copying the headers is sufficient.


```powershell
git clone https://github.com/Neargye/magic_enum.git /tmp/magic_enum
cd /tmp/magic_enum
git checkout v0.9.7
sudo mkdir -p /usr/local/include/magic_enum
sudo cp include/magic_enum/*.hpp /usr/local/include/magic_enum/
```

### Exiv2
```powershell
sudo apt install -y \
         ccache \
         libbrotli-dev \
         libexpat1-dev \
         libgtest-dev \
         libinih-dev \
         libssh-dev \
         libxml2-utils \
         libz-dev \
         zlib1g-dev
        shell: bash
```

Build :

```powershell
git clone https://github.com/Exiv2/exiv2.git /tmp/exiv2-src
cd /tmp/exiv2-src
git checkout v0.28.7
    
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=${{ matrix.build-type }} \
        -DCMAKE_INSTALL_PREFIX=/opt/exiv2 \
        -DBUILD_SHARED_LIBS=ON \
        -DBUILD_WITH_CCACHE=ON \
        ..
    
cmake --build . --parallel $(nproc)
sudo cmake --install .
    
echo "CMAKE_PREFIX_PATH=/opt/exiv2:$CMAKE_PREFIX_PATH" >> $GITHUB_ENV
```

## 🚀Build the project
* You can remove desktio_ui if you don't want to compile the ui
* You can also use debug-ninja-linux instead of release-ninja-linux
```powershell
# Use a generic preset that doesn't rely on specific paths (like *-homebrew presets)
# Enable the desktop UI if desired
cmake --preset release-ninja -DBUILD_DESKTOP_UI=ON

# Or, for a debug build:
# cmake --preset debug-ninja -DBUILD_DESKTOP_UI=ON

# Compile using all available cores
cmake --build build/release-ninja -j$(nproc) # Adjust path if using debug preset
```

**Build Commands**

Since dependencies are installed system-wide, we use generic presets. Don't forget to enable the UI if you wanna to build it!

```powershell
# Configure in Debug and enable the UI
cmake --preset debug -Ddesktop_ui=ON
# Compile using all available cores
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
| release-ninja-linux | Ninja | Release    | Classic Makefiles generator.                     |
| debug-ninja-linux  | Ninja | Debug      | Classic Makefiles generator.                     |