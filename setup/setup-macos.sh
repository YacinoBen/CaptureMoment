#!/bin/bash

# ==============================================================================
# CaptureMoment - macOS Local Setup Script
# 
# This script compiles dependencies from source to guarantee strict version
# control, matching the exact versions used in the CI and Linux builds.
# 
# ==============================================================================

set -e # Exit immediately if a command exits with a non-zero status

echo "=============================================================================="
echo "WARNING: This script does NOT install Qt."
echo "Please make sure you have installed Qt manually and set your CMAKE_PREFIX_PATH"
echo "environment variable accordingly before building the project."
echo "=============================================================================="
echo ""

# ==============================================================================
# System dependencies (via Homebrew)
# ==============================================================================
echo "[1/5] Installing base build tools via Homebrew..."
brew update
brew install cmake ninja ccache

# Install the underlying libraries required to compile Exiv2 and OpenImageIO.
# Homebrew handles these well, and we don't need strict version control on them.
echo "[2/5] Installing underlying system libraries via Homebrew..."
brew install \
    spdlog libcurl xkbcommon \
    brotli expat googletest inih libssh xmlstarlet zlib jpeg freetype png \
    libuhd libraw opencv jxl libheif pystring tiff imath openexr \
    yaml-cpp halide

# ==============================================================================
# Build and install Exiv2
# ==============================================================================
echo "[4/5] Building and installing Exiv2 v0.28.7..."
if [[ ! -d "/opt/exiv2" ]]; then
    git clone https://github.com/Exiv2/exiv2.git /tmp/exiv2-src
    cd /tmp/exiv2-src && git checkout v0.28.7
    
    # On macOS, we need to tell CMake where Homebrew libraries are if they are keg-only
    BREW_PREFIX=$(brew --prefix)
    
    cmake -S . -B build -G Ninja \
          -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_INSTALL_PREFIX=/opt/exiv2 \
          -DCMAKE_PREFIX_PATH="${BREW_PREFIX}" \
          -DBUILD_SHARED_LIBS=ON
    
    cmake --build build --parallel $(sysctl -n hw.ncpu)
    sudo cmake --install build
    rm -rf /tmp/exiv2-src
    echo "      -> Exiv2 installed successfully in /opt/exiv2."
else
    echo "      -> Exiv2 already found in /opt/exiv2, skipping."
fi

# ==============================================================================
# Build and install OpenImageIO
# ==============================================================================
echo "[5/5] Building and installing OpenImageIO v3.1.8.0..."
if [[ ! -d "/opt/oiio" ]]; then
    git clone https://github.com/AcademySoftwareFoundation/OpenImageIO.git /tmp/oiio-src
    cd /tmp/oiio-src && git checkout v3.1.8.0
    
    BREW_PREFIX=$(brew --prefix)
    
    mkdir build && cd build
    cmake -DCMAKE_BUILD_TYPE=Release \
           -DCMAKE_INSTALL_PREFIX=/opt/oiio \
           -DCMAKE_PREFIX_PATH="/opt/exiv2;${BREW_PREFIX}" \
           -DOpenImageIO_BUILD_MISSING_DEPS=required \
           -DSTOP_ON_WARNING=0 \
           -DOIIO_BUILD_TOOLS=OFF \
           -DOIIO_BUILD_TESTS=OFF \
           -DUSE_PYTHON=OFF \
           ..
           
    cmake --build . --parallel $(sysctl -n hw.ncpu)
    sudo cmake --install .
    rm -rf /tmp/oiio-src
    echo "      -> OpenImageIO installed successfully in /opt/oiio."
else
    echo "      -> OpenImageIO already found in /opt/oiio, skipping."
fi

# ==============================================================================
# Configuration for CMake and library paths
# ==============================================================================
echo ""
echo "=============================================================================="
echo "Configuring environment variables..."
export CMAKE_PREFIX_PATH="/opt/exiv2:/opt/oiio:${CMAKE_PREFIX_PATH}"
export DYLD_LIBRARY_PATH="/opt/exiv2/lib:/opt/oiio/lib:${DYLD_LIBRARY_PATH}" # DYLD instead of LD on macOS

echo "To make these variables persistent, add the following lines to your ~/.zshrc:"
echo "  export CMAKE_PREFIX_PATH=\"/opt/exiv2:/opt/oiio:\${CMAKE_PREFIX_PATH}\""
echo "  export DYLD_LIBRARY_PATH=\"/opt/exiv2/lib:/opt/oiio/lib:\${DYLD_LIBRARY_PATH}\""
echo "=============================================================================="
echo ""
echo "✅ Setup completed successfully! You can now build CaptureMoment."
