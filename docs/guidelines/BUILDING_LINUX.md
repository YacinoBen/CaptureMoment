### `docs/build/BUILDING_LINUX.md`
*Detailed Linux guide addressing LLVM build times and disk space.*


# 🐧 Linux Building Guide
On Linux, you can choose between using system packages (faster) or Vcpkg (isolated environment).

## 📦 Method 1: System Packages (Recommended)

This is the fastest method as it uses pre-compiled binaries via your distribution's package manager, thus avoiding the long LLVM compilation time.

### Ubuntu 24.04+ / Debian 12+

### Build OpenImageIO
Make sure you have : ninja, cmake and Qt6.9.3 if you want to compile the UI
#### Install the libraries
```powershell
sudo apt install -y libspdlog-dev 

# If you can compile cmake with halide installed with pip ok, otherwise delete it and
# Install libhalide-dev, generally in ubuntu 24 it's 17-1, but in Ubuntu 25 maybe you need to change the version.
pip uninstall halide -y halide || true

# Execute the following if you don't have halide or pip halide can't 
# compile the project with cmake
sudo apt install -y libhalide17-1 libhalide17-1-dev python3-halide

# Better to use install Qt via Qt online installer to chose 6.9.3
# but you can use sudo apt install -y qt6-base-dev qt6-declarative-dev , but check the version installed

sudo apt install -y libcurl4-openssl-dev
sudo apt install -y libxkbcommon-dev
```
#### Compile OpenImageIO
```powershell
sudo apt update
sudo apt build-essential 
```

##### Automatic build
You can install directly OIIO, but check you have the v3.1.8.0, because in ubuntu 24, the default package is 2.5, make sure you can upgrade or build it manually
```powershell
sudo apt libopenimageio-dev
```
##### Manual build

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

### Compilation
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

### Build the project

* You can remove desktio_ui if you don't want to compile the ui
* Youcan also use debug-ninja-linux instead of release-ninja-linux
```powershell
cmake \
--preset release-ninja-linux \
          -DBUILD_DESKTOP_UI:BOOL=ON \
          -DCMAKE_BUILD_TYPE=release-ninja-linux
```
## Fedora 40+ (not tested)
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


## 📦Method 2: Vcpkg (Isolated) (nt tested)

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
| release-ninja-linux | Ninja | Release    | Classic Makefiles generator.                     |
| debug-ninja-linux  | Ninja | Debug      | Classic Makefiles generator.                     |