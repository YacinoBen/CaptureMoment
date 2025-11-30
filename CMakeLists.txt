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
# Research dependencis with fallback
# ============================================================

find_package(OpenImageIO CONFIG QUIET)
if(NOT OpenImageIO_FOUND)
    find_package(OpenImageIO REQUIRED) # Fallback in FindOpenImageIO.cmake
endif()

if(OpenImageIO_FOUND)
    message(STATUS "OpenImageIO found : ${OpenImageIO_VERSION}")
else()
    message(FATAL_ERROR "OpenImageIO not found. Please install it with :\n"
        "  - Linux (apt) : sudo apt install libopenimageio-dev\n"
        "  - macOS (brew) : brew install openimageio\n"
        "  - Windows (vcpkg) : vcpkg install openimageio\n"
        "  - Windows (Manifest vcpkg) : vcpkg install\n"
        "  - Or manual compilation : https://github.com/AcademySoftwareFoundation/OpenImageIO")
endif()


if(Halide_FOUND)
    message(STATUS "Halide found : ${Halide_VERSION}")
else()
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

# core/, catalog/, color/, etc. Later :
# add_subdirectory(core)
# add_subdirectory(catalog)
# add_subdirectory(color)