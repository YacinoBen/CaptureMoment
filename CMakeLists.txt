cmake_minimum_required(VERSION 3.21)

project(CaptureMoment VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ============================================================
# Include CMake modules
# ============================================================
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(CompilerDetection)
include(VcpkgHelpers)
include(PackageHelpers)

# ============================================================
# Verify C++23 support
# ============================================================
verify_cpp23_support() # From CompilerDetection.cmake

# ============================================================
# Build options
# ============================================================
option(BUILD_DESKTOP_UI "Build desktop UI" OFF)
option(BUILD_MOBILE_UI "Build mobile UI" OFF)
option(BUILD_TESTS "Build tests" OFF)
option(BUILD_BENCHMARKS "Build benchmarks" OFF)
option(BUILD_EXAMPLES "Build examples" OFF)

# Aliases
option(desktop_ui "Alias for BUILD_DESKTOP_UI" OFF)
option(mobile_ui "Alias for BUILD_MOBILE_UI" OFF)
option(tests "Alias for BUILD_TESTS" OFF)
option(benchmarks "Alias for BUILD_BENCHMARKS" OFF)
option(examples "Alias for BUILD_EXAMPLES" OFF)

# Synchronize aliases
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
if(examples)
    set(BUILD_EXAMPLES ON)
endif()

# ============================================================
# Configure vcpkg (if detected)
# ============================================================
configure_vcpkg_if_present() # From VcpkgHelpers.cmake

# ============================================================
# Find dependencies
# ============================================================
find_required_packages() # From PackageHelpers.cmake

# ============================================================
# Global configuration
# ============================================================
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/config/config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/config.h"
)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

# ============================================================
# Subprojects
# ============================================================

add_subdirectory(core)

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

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# ============================================================
# Apply compiler flags to ALL targets globally
# ============================================================
apply_compiler_flags_recursive(${CMAKE_CURRENT_SOURCE_DIR}) # From CompilerDetection.cmake

# ============================================================
# Configuration summary
# ============================================================
get_compiler_name(COMPILER_NAME) # From CompilerDetection.cmake

message(STATUS "")
message(STATUS "╔════════════════════════════════════════════════════════════╗")
message(STATUS "║  CaptureMoment ${PROJECT_VERSION} Configuration             ")
message(STATUS "╠════════════════════════════════════════════════════════════╣")
message(STATUS "║  Compiler        : ${COMPILER_NAME}")
message(STATUS "║  Build Type      : ${CMAKE_BUILD_TYPE}")
message(STATUS "║  C++ Standard    : C++${CMAKE_CXX_STANDARD}")
if(DEFINED VCPKG_TARGET_TRIPLET)
message(STATUS "║  vcpkg Triplet   : ${VCPKG_TARGET_TRIPLET}")
endif()
message(STATUS "╠════════════════════════════════════════════════════════════╣")
message(STATUS "║  Build Options:                                            ║")
message(STATUS "║    Desktop UI    : ${BUILD_DESKTOP_UI}")
message(STATUS "║    Mobile UI     : ${BUILD_MOBILE_UI}")
message(STATUS "║    Tests         : ${BUILD_TESTS}")
message(STATUS "║    Benchmarks    : ${BUILD_BENCHMARKS}")
message(STATUS "║    Examples      : ${BUILD_EXAMPLES}")
message(STATUS "╚════════════════════════════════════════════════════════════╝")
message(STATUS "")