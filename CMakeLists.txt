cmake_minimum_required(VERSION 3.21)

project(CaptureMoment VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ============================================================
# Build options
# ============================================================
option(BUILD_DESKTOP_UI "Build desktop UI" OFF)
option(BUILD_MOBILE_UI "Build mobile UI" OFF) 
option(BUILD_TESTS "Build tests" OFF)
option(BUILD_BENCHMARKS "Build benchmarks " OFF)
option(AUTO_DETECT_TOOLCHAIN "Automatically detect compiler and toolchain" OFF)
option(USE_PREBUILT_BINARIES "Use prebuilt binaries if available" ON)

# Alias options for easier command line usage
option(desktop_ui "Build desktop UI (alias for BUILD_DESKTOP_UI)" OFF)
option(mobile_ui "Build mobile UI (alias for BUILD_MOBILE_UI)" OFF)
option(tests "Build tests (alias for BUILD_TESTS)" OFF)
option(benchmarks "Build benchmarks (alias for BUILD_BENCHMARKS)" OFF)
    

# We synchronize the aliases with the actual options
# If the alias is ON, the actual option becomes ON (and vice versa)
if(desktop_ui)
    set(BUILD_DESKTOP_UI ON)
endif()

if(mobile_ui)
    set(BUILD_MOBILE_UI ON)
endif()

if(tests)
    set(BUILD_TESTS ON)
endif()

if(benchmarks)
    set(BUILD_BENCHMARKS ON)
endif()

if(DEFINED desktop_ui OR DEFINED mobile_ui OR DEFINED tests OR DEFINED benchmarks)
    message(STATUS "Alias used : desktop_ui=${desktop_ui}, mobile_ui=${mobile_ui}, tests=${tests}, benchmarks=${benchmarks}")
    message(STATUS "Actual values : BUILD_DESKTOP_UI=${BUILD_DESKTOP_UI}, BUILD_MOBILE_UI=${BUILD_MOBILE_UI}, BUILD_TESTS=${BUILD_TESTS}, BUILD_BENCHMARKS=${BUILD_BENCHMARKS}")
endif()

# ============================================================
# Auto detect compiler and toolchain
# ============================================================
function(detect_compiler_toolchain)
    if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        set(DETECTED_TOOLCHAIN "MSVC" PARENT_SCOPE)
        message(STATUS "🔍 Compiler detected: MSVC")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        set(DETECTED_TOOLCHAIN "MinGW" PARENT_SCOPE)
        message(STATUS "🔍 Compiler detected: MinGW/GCC")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(DETECTED_TOOLCHAIN "Clang" PARENT_SCOPE)
        message(STATUS "🔍 Compiler detected: Clang")
    else()
        set(DETECTED_TOOLCHAIN "Unknown" PARENT_SCOPE)
        message(WARNING "⚠️  Compiler not recognized: ${CMAKE_CXX_COMPILER_ID}")
    endif()
endfunction()

# ============================================================
# Package manager configuration
# ============================================================

# Vcpkg specific configuration
if(DEFINED CMAKE_TOOLCHAIN_FILE AND "${CMAKE_TOOLCHAIN_FILE}" MATCHES "vcpkg.cmake$")
    message(STATUS "📦 vcpkg detected : ${CMAKE_TOOLCHAIN_FILE}")
    
    # Automatic toolchain detection if requested
    if(AUTO_DETECT_TOOLCHAIN)
        detect_compiler_toolchain()
        
        if(NOT DEFINED VCPKG_TARGET_TRIPLET)
            # Automatic determination of the triplet based on the compiler and build type
            if(DETECTED_TOOLCHAIN STREQUAL "MSVC")
                if(CMAKE_BUILD_TYPE STREQUAL "Debug")
                    set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "Triplet vcpkg auto-detected")
                    message(STATUS "✅ Triplet auto-selected: x64-windows (MSVC Debug)")
                else()
                    set(VCPKG_TARGET_TRIPLET "x64-windows-release" CACHE STRING "Triplet vcpkg auto-detected")
                    message(STATUS "✅ Triplet auto-selected: x64-windows-release (MSVC Release)")
                endif()
            elseif(DETECTED_TOOLCHAIN STREQUAL "MinGW")
                if(CMAKE_BUILD_TYPE STREQUAL "Debug")
                    set(VCPKG_TARGET_TRIPLET "x64-mingw-dynamic" CACHE STRING "Triplet vcpkg auto-detected")
                    message(STATUS "✅ Triplet auto-selected: x64-mingw-dynamic (MinGW Debug)")
                else()
                    set(VCPKG_TARGET_TRIPLET "x64-mingw-dynamic-release" CACHE STRING "Triplet vcpkg auto-detected")
                    message(STATUS "✅ Triplet auto-selected : x64-mingw-dynamic-release (MinGW Release)")
                endif()
            else()
                message(WARNING "⚠️  Toolchain not supported for auto-detection, using x64-windows by default")
                set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "Triplet vcpkg default")
            endif()
        endif()
    else()
        # Manual mode: check defined triplet
        if(NOT DEFINED VCPKG_TARGET_TRIPLET)
            # Default value if not specified
            set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "Triplet vcpkg (forced default)")
            message(STATUS "⚙️  VCPKG_TARGET_TRIPLET not defined, using '${VCPKG_TARGET_TRIPLET}' by default")
        else()
            message(STATUS "⚙️  Using specified VCPKG_TARGET_TRIPLET: '${VCPKG_TARGET_TRIPLET}'")
        endif()
    endif()
    
    # Consistency check
    detect_compiler_toolchain()
    
    if(DETECTED_TOOLCHAIN STREQUAL "MSVC" AND NOT VCPKG_TARGET_TRIPLET MATCHES "x64-windows")
        message(WARNING "⚠️  Inconsistency detected: MSVC compiler but triplet '${VCPKG_TARGET_TRIPLET}'. Build issues may occur.")
    elseif(DETECTED_TOOLCHAIN STREQUAL "MinGW" AND NOT VCPKG_TARGET_TRIPLET MATCHES "mingw")
        message(WARNING "⚠️  Inconsistency detected: MinGW/GCC compiler but triplet '${VCPKG_TARGET_TRIPLET}'. Build issues may occur.")
    endif()
    
    message(STATUS "🎯 Final configuration :")
    message(STATUS "   - Compiler : ${DETECTED_TOOLCHAIN} (${CMAKE_CXX_COMPILER_ID})")
    message(STATUS "   - Build Type : ${CMAKE_BUILD_TYPE}")
    message(STATUS "   - Vcpkg triplet : ${VCPKG_TARGET_TRIPLET}")
endif()

# ============================================================
# Function to download prebuilt binaries
# ============================================================
function(try_download_prebuilt_package PACKAGE_NAME)
    if(NOT USE_PREBUILT_BINARIES)
        return()
    endif()
    
    message(STATUS "🔍 Searching for prebuilt binaries for ${PACKAGE_NAME}...")
    
    # Example: NuGet, GitHub Releases, or vcpkg binary cache
    # This function can be extended as needed
    
    # For vcpkg, we can enable binary caching
    if(DEFINED CMAKE_TOOLCHAIN_FILE AND "${CMAKE_TOOLCHAIN_FILE}" MATCHES "vcpkg.cmake$")
        if(NOT DEFINED VCPKG_BINARY_SOURCES)
            set(VCPKG_BINARY_SOURCES "clear;nuget,https://nuget.pkg.github.com/microsoft/index.json,readwrite" CACHE STRING "Vcpkg binary sources")
            message(STATUS "📥 Vcpkg binary caching enabled for ${PACKAGE_NAME}")
        endif()
    endif()
endfunction()

#============================================================
# Interactive function to ask the user
#============================================================
function(ask_user_for_build_method PACKAGE_NAME)
    message(STATUS "")
    message(STATUS "╔════════════════════════════════════════════════════════════╗")
    message(STATUS "║  ${PACKAGE_NAME} not found!                                ")
    message(STATUS "╠════════════════════════════════════════════════════════════╣")
    message(STATUS "║  Available options:                                        ")
    message(STATUS "║                                                            ")
    message(STATUS "║  1. Download prebuilt binaries (Fast)                      ")
    message(STATUS "║     └─> Recommended to save time                           ")
    message(STATUS "║                                                            ")
    message(STATUS "║  2. Build from sources with vcpkg (Slow)                   ")
    message(STATUS "║     └─> May take several hours for LLVM                    ")
    message(STATUS "║                                                            ")
    message(STATUS "║  3. Install manually                                       ")
    message(STATUS "║     └─> You will need to install ${PACKAGE_NAME} yourself  ")
    message(STATUS "╚════════════════════════════════════════════════════════════╝")
    message(STATUS "")
    message(STATUS "To choose an option, rerun CMake with:")
    message(STATUS "  -DUSE_PREBUILT_BINARIES=ON   (Option 1)")
    message(STATUS "  -DUSE_PREBUILT_BINARIES=OFF  (Option 2)")
    message(STATUS "")
endfunction()

# ============================================================
# Research dependencis with fallback
# ============================================================
# --- OpenImageIO ---
message(STATUS "🔍 Searching for OpenImageIO...")

find_package(OpenImageIO CONFIG QUIET)
if(NOT OpenImageIO_FOUND)
    message(STATUS "⚠️  OpenImageIO not found via CONFIG, trying MODULE...")
    find_package(OpenImageIO MODULE REQUIRED)
endif()

if(OpenImageIO_FOUND)
    message(STATUS "OpenImageIO found : ${OpenImageIO_VERSION}")
else()
    ask_user_for_build_method("OpenImageIO")
    try_download_prebuilt_package("OpenImageIO")
    message(FATAL_ERROR "OpenImageIO not found. Please install it with :\n"
        "  - Linux (apt) : sudo apt install libopenimageio-dev\n"
        "  - macOS (brew) : brew install openimageio\n"
        "  - Windows (vcpkg) : vcpkg install openimageio\n"
        "  - Windows (Manifest vcpkg) : vcpkg install\n"
        "  - Or manual compilation : https://github.com/AcademySoftwareFoundation/OpenImageIO")
endif()

# --- Halide ---
message(STATUS "🔍 Searching for Halide...")
find_package(Halide CONFIG QUIET)

if(Halide_FOUND)
    message(STATUS "Halide found : ${Halide_VERSION}")
else()
    ask_user_for_build_method("Halide")
    try_download_prebuilt_package("Halide")

        message(STATUS "")
    message(STATUS "⚠️  ATTENTION : Halide need LLVM, which is very large (~50 GB)")
    message(STATUS "    If you compile from sources, make sure you have :")
    message(STATUS "    - At least 100 GB of free disk space")
    message(STATUS "    - 16 GB of RAM minimum (32 GB recommended)")
    message(STATUS "    - Several hours of compilation time")
    message(STATUS "")
    message(STATUS "💡 RECOMMENDATION : Use option 1 (prebuilt binaries)")
    message(STATUS "")

    message(FATAL_ERROR "Halide not found. Please install it with :\n"
        "  - Linux (apt) : sudo apt install libhalide-dev\n"
        "  - macOS (brew) : brew install halide\n"
        "  - Windows (vcpkg) : vcpkg install halide\n"
        "  - Windows (Manifest vcpkg) : vcpkg install\n"
        "  - Or manual compilation : https://github.com/AcademySoftwareFoundation/OpenImageIO")
endif()
# ============================================================
# Global Configuration
# ============================================================
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/config/config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/config.h"
)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

# ============================================================
# Subprojects
# ============================================================

if(BUILD_DESKTOP_UI)
    add_subdirectory(ui/desktop)
endif()

if(BUILD_MOBILE_UI)
    add_subdirectory(ui/mobile)
endif()

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

add_compile_definitions(CAPTUREMOMENT_VERSION="${PROJECT_VERSION}")

# Resume of the configuration
message(STATUS "")
message(STATUS "╔════════════════════════════════════════════════════════════╗")
message(STATUS "║  Configuration CaptureMoment ${PROJECT_VERSION}              ")
message(STATUS "╠════════════════════════════════════════════════════════════╣")
message(STATUS "║  Compilator     : ${CMAKE_CXX_COMPILER_ID}                ")
message(STATUS "║  Build Type      : ${CMAKE_BUILD_TYPE}                     ")
message(STATUS "║  Standard C++    : ${CMAKE_CXX_STANDARD}                   ")
if(DEFINED VCPKG_TARGET_TRIPLET)
message(STATUS "║  Triplet vcpkg   : ${VCPKG_TARGET_TRIPLET}                 ")
endif()
message(STATUS "╠════════════════════════════════════════════════════════════╣")
message(STATUS "║  Build options :                                        ")
message(STATUS "║    Desktop UI    : ${BUILD_DESKTOP_UI}                     ")
message(STATUS "║    Mobile UI     : ${BUILD_MOBILE_UI}                      ")
message(STATUS "║    Tests         : ${BUILD_TESTS}                          ")
message(STATUS "║    Benchmarks    : ${BUILD_BENCHMARKS}                     ")
message(STATUS "╚════════════════════════════════════════════════════════════╝")
message(STATUS "")
# core/, catalog/, color/, etc. Later :
# add_subdirectory(core)
# add_subdirectory(catalog)
# add_subdirectory(color)