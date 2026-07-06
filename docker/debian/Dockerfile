FROM gcc:16.1.0

ENV DEBIAN_FRONTEND=noninteractive

# ==============================================================================
# System dependencies
# ==============================================================================
RUN apt-get update && apt-get install -y \
    git ninja-build python3 python3-pip python3-venv ccache \
    libspdlog-dev libcurl4-openssl-dev libxkbcommon-dev libgl1-mesa-dev \
    libbrotli-dev libexpat1-dev libgtest-dev libinih-dev libssh-dev \
    libxml2-utils libz-dev zlib1g-dev libjpeg-dev libfreetype-dev \
    libpng-dev libuhd-dev libraw-dev libopencv-core-dev libopencv-imgproc-dev \
    libjxl-dev libheif-dev libpystring-dev libtiff-dev libimath-dev \
    libopenexr-dev \
    libyaml-cpp-dev \
    && rm -rf /var/lib/apt/lists/*

# ==============================================================================
# Install Python core packages
# ==============================================================================
RUN pip3 install --break-system-packages cmake aqtinstall halide

# ==============================================================================
# Install Qt framework
# ==============================================================================
RUN aqt install-qt linux desktop 6.10.2 --outputdir /opt/qt -m qtdeclarative -m qtshadertools

# ==============================================================================
# Configuration for CMake and library paths
# ==============================================================================
ENV CMAKE_PREFIX_PATH="/opt/qt/6.10.2/gcc_64;/opt/exiv2;/opt/oiio"
ENV LD_LIBRARY_PATH="/opt/exiv2/lib:/opt/oiio/lib"

# ==============================================================================
# Build and install Exiv2
# ==============================================================================
RUN git clone https://github.com/Exiv2/exiv2.git /tmp/exiv2-src && \
    cd /tmp/exiv2-src && git checkout v0.28.7 && \
    cmake -S . -B build -G Ninja \
          -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_INSTALL_PREFIX=/opt/exiv2 \
          -DBUILD_SHARED_LIBS=ON && \
    cmake --build build --parallel $(nproc) && \
    cmake --install build && \
    rm -rf /tmp/*

# ==============================================================================
# Build and install OpenImageIO
# ==============================================================================
RUN git clone https://github.com/AcademySoftwareFoundation/OpenImageIO.git /tmp/oiio-src && \
    cd /tmp/oiio-src && git checkout v3.1.8.0 && \
    mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release \
           -DCMAKE_INSTALL_PREFIX=/opt/oiio \
           -DOpenImageIO_BUILD_MISSING_DEPS=required \
           -DSTOP_ON_WARNING=0 \
           -DOIIO_BUILD_TOOLS=OFF \
           -DOIIO_BUILD_TESTS=OFF \
           -DUSE_PYTHON=OFF \
           .. && \
    cmake --build . --parallel $(nproc) && \
    cmake --install . && \
    rm -rf /tmp/*
WORKDIR /app