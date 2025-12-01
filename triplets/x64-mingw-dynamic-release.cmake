# Personalized triplet for Release build only with MinGW
# File: triplets/x64-mingw-dynamic-release.cmake

set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)

# Force Release build only
set(VCPKG_BUILD_TYPE release)

# Platform MinGW
set(VCPKG_CMAKE_SYSTEM_NAME MinGW)
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/toolchains/mingw.cmake")

# MinGW compilers
set(VCPKG_CXX_FLAGS "-O3 -march=native")
set(VCPKG_C_FLAGS "-O3 -march=native")

# Disable tests, examples, and docs to save space
set(VCPKG_CMAKE_CONFIGURE_OPTIONS 
    -DBUILD_TESTING=OFF 
    -DBUILD_EXAMPLES=OFF
    -DBUILD_DOCS=OFF
    -DBUILD_SHARED_LIBS=ON
)

# Specific options for Halide
if(PORT MATCHES "halide")
    list(APPEND VCPKG_CMAKE_CONFIGURE_OPTIONS
        -DHALIDE_SHARED_LIBRARY=ON
        -DWITH_TESTS=OFF
        -DWITH_TUTORIALS=OFF
        -DWITH_DOCS=OFF
    )
endif()

# Options for OpenImageIO (minimal)
if(PORT MATCHES "openimageio")
    list(APPEND VCPKG_CMAKE_CONFIGURE_OPTIONS
        -DUSE_PYTHON=OFF
        -DBUILD_TESTING=OFF
        -DOIIO_BUILD_TOOLS=OFF
        -DOIIO_BUILD_TESTS=OFF
    )
endif()

# Options for LLVM (if built)
if(PORT MATCHES "llvm")
    list(APPEND VCPKG_CMAKE_CONFIGURE_OPTIONS
        -DLLVM_INCLUDE_TESTS=OFF
        -DLLVM_INCLUDE_EXAMPLES=OFF
        -DLLVM_INCLUDE_DOCS=OFF
        -DLLVM_BUILD_TOOLS=ON
        -DLLVM_ENABLE_ASSERTIONS=OFF
    )
endif()