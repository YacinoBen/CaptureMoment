# Core library for CaptureMoment
# The root CMakeLists.txt already found and verified all dependencies
# via PackageHelpers.cmake - we just link them here

cmake_minimum_required(VERSION 3.25)

# Debug: Show current directory
message(STATUS "Starting configuration of capturemoment_core...")
message(STATUS "[core] CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")

# Source files (only .cpp files that get compiled)
set(CORE_SOURCES
    src/image_region.cpp
    src/source_manager.cpp
    src/pipeline_engine.cpp
    src/operations/operation_factory.cpp
    src/operations/operation_brightness.cpp
)

# Verify sources exist
foreach(source ${CORE_SOURCES})
    set(full_path "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
    if(NOT EXISTS "${full_path}")
        message(FATAL_ERROR "[core] Source file not found: ${full_path}")
    endif()
endforeach()

# Create library
add_library(capturemoment_core STATIC ${CORE_SOURCES})

# Add headers for IDE
file(GLOB_RECURSE HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
if(HEADER_FILES)
    set_source_files_properties(${HEADER_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)
    target_sources(capturemoment_core PRIVATE ${HEADER_FILES})
endif()

# Link dependencies (already verified by root CMakeLists via PackageHelpers)
# If we reach here, packages are guaranteed to exist
target_link_libraries(capturemoment_core PUBLIC
    spdlog::spdlog
    OpenImageIO::OpenImageIO
    Halide::Halide
)

# Include directories
target_include_directories(capturemoment_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# C++23 required
target_compile_features(capturemoment_core PUBLIC cxx_std_23)

# Compiler flags (centralized in CompilerDetection.cmake)
apply_compiler_flags(capturemoment_core)

# Properties
set_target_properties(capturemoment_core PROPERTIES
    VERSION ${PROJECT_VERSION}
    POSITION_INDEPENDENT_CODE ON
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# ============================================================
# Core configuration summary
# ============================================================
message(STATUS "")
message(STATUS "╔════════════════════════════════════════════════════════════╗")
message(STATUS "║  [core] capturemoment_core configured                       ║")
message(STATUS "╠════════════════════════════════════════════════════════════╣")
message(STATUS "║  Type              : Static Library (STATIC)")
message(STATUS "║  Source Files      : ${CORE_SOURCES}")
message(STATUS "║  Header Files      : ${HEADER_FILES}")
message(STATUS "║  Output Directory  : ${CMAKE_BINARY_DIR}/lib")
message(STATUS "╚════════════════════════════════════════════════════════════╝")
message(STATUS "")