#!/bin/bash

# ==============================================================================
# CaptureMoment - Linux Local Setup Script
# 
# This script automates the setup of the build environment and compiles the
# required core dependencies (Exiv2, OpenImageIO, magic_enum) from source.
# 
# NOTE: Qt is NOT installed by this script. You must install Qt manually
# and ensure CMAKE_PREFIX_PATH points to your Qt installation before building.
# ==============================================================================

set -e # Exit immediately if a command exits with a non-zero status

echo "=============================================================================="
echo "WARNING: This script does NOT install Qt."
echo "Please make sure you have installed Qt manually and set your CMAKE_PREFIX_PATH"
echo "environment variable accordingly before building the project."
echo "=============================================================================="
echo ""

# ==============================================================================
# System dependencies
# ==============================================================================
echo "[1/5] Installing system dependencies..."
sudo apt-get update && sudo apt-get install -y \
    git ninja-build python3 python3-pip python3-venv ccache \
    libspdlog-dev libcurl4-openssl-dev libxkbcommon-dev libgl1-mesa-dev \
    libbrotli-dev libexpat1-dev libgtest-dev libinih-dev libssh-dev \
    libxml2-utils libz-dev zlib1g-dev libjpeg-dev libfreetype-dev \
    libpng-dev libuhd-dev libraw-dev libopencv-core-dev libopencv-imgproc-dev \
    libjxl-dev libheif-dev libpystring-dev libtiff-dev libimath-dev \
    libopenexr-dev \
    libyaml-cpp-dev

# ==============================================================================
# Install Python core packages
# ==============================================================================
echo "[2/5] Installing Python core packages..."
pip3 install --break-system-packages cmake halide

# ==============================================================================
# Install magic_enum (Header-only)
# ==============================================================================
echo "[3/5] Installing magic_enum..."
if [ ! -d "/usr/local/include/magic_enum" ]; then
    git clone https://github.com/Neargye/magic_enum.git /tmp/magic_enum
    cd /tmp/magic_enum && git checkout v0.9.7
    sudo mkdir -p /usr/local/include/magic_enum
    sudo cp include/magic_enum/*.hpp /usr/local/include/magic_enum/
    rm -rf /tmp/magic_enum
    echo "      -> magic_enum installed successfully."
else
    echo "      -> magic_enum already found in /usr/local/include/magic_enum, skipping."
fi

# ==============================================================================
# Build and install Exiv2
# ==============================================================================
echo "[4/5] Building and installing Exiv2 v0.28.7..."
if [ ! -d "/opt/exiv2" ]; then
    git clone https://github.com/Exiv2/exiv2.git /tmp/exiv2-src
    cd /tmp/exiv2-src && git checkout v0.28.7
    
    cmake -S . -B build -G Ninja \
          -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_INSTALL_PREFIX=/opt/exiv2 \
          -DBUILD_SHARED_LIBS=ON
    
    cmake --build build --parallel $(nproc)
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
if [ ! -d "/opt/oiio" ]; then
    git clone https://github.com/AcademySoftwareFoundation/OpenImageIO.git /tmp/oiio-src
    cd /tmp/oiio-src && git checkout v3.1.8.0
    
    mkdir build && cd build
    cmake -DCMAKE_BUILD_TYPE=Release \
           -DCMAKE_INSTALL_PREFIX=/opt/oiio \
           -DOpenImageIO_BUILD_MISSING_DEPS=required \
           -DSTOP_ON_WARNING=0 \
           -DOIIO_BUILD_TOOLS=OFF \
           -DOIIO_BUILD_TESTS=OFF \
           -DUSE_PYTHON=OFF \
           ..
           
    cmake --build . --parallel $(nproc)
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
export LD_LIBRARY_PATH="/opt/exiv2/lib:/opt/oiio/lib:${LD_LIBRARY_PATH}"

echo "To make these variables persistent, add the following lines to your ~/.bashrc:"
echo "  export CMAKE_PREFIX_PATH=\"/opt/exiv2:/opt/oiio:\${CMAKE_PREFIX_PATH}\""
echo "  export LD_LIBRARY_PATH=\"/opt/exiv2/lib:/opt/oiio/lib:\${LD_LIBRARY_PATH}\""
echo "=============================================================================="
echo ""
echo "✅ Setup completed successfully! You can now build CaptureMoment."
