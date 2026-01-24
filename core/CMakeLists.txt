# Core library for CaptureMoment
# The root CMakeLists.txt already found and verified all dependencies
# via PackageHelpers.cmake - we just link them here

cmake_minimum_required(VERSION 3.25)

# Debug: Show current directory
message(STATUS "Starting configuration of capturemoment_core...")
message(STATUS "[core] CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")

# Source files (only .cpp files that get compiled)
set(CORE_SOURCES
    # Manager
    src/managers/source_manager.cpp
    src/managers/state_image_manager.cpp

    # Operations
    src/operations/operation_factory.cpp
    src/operations/operation_pipeline.cpp
    src/operations/operation_registry.cpp

    # Image Processing
    src/image_processing/cpu/working_image_cpu_default.cpp
    src/image_processing/cpu/working_image_cpu_halide.cpp
    src/image_processing/gpu/working_image_gpu_halide.cpp
    src/image_processing/factories/working_image_factory.cpp
    src/image_processing/halide/working_image_halide.cpp

    # Deciders
    src/image_processing/deciders/benchmarking_backend_decider.cpp

    # Config
    src/config/app_config.cpp

    # Serializer
    src/serializer/provider/exiv2_initializer.cpp
    src/serializer/provider/exiv2_provider.cpp
    src/serializer/strategy/appdata_xmp_path_strategy.cpp
    src/serializer/strategy/configurable_xmp_path_strategy.cpp
    src/serializer/strategy/sidecar_xmp_path_strategy.cpp

    # Pipeline
    src/pipeline/operation_pipeline_executor.cpp
    src/pipeline/operation_pipeline_builder.cpp

    src/serializer/file_serializer_manager.cpp
    src/serializer/operation_serialization.cpp
    src/serializer/file_serializer_writer.cpp
    src/serializer/file_serializer_reader.cpp

    # List Operations base adjustment
    src/operations/basic_adjustment_operations/operation_brightness.cpp
    src/operations/basic_adjustment_operations/operation_constrast.cpp
    src/operations/basic_adjustment_operations/operation_highlights.cpp
    src/operations/basic_adjustment_operations/operation_shadows.cpp
    src/operations/basic_adjustment_operations/operation_whites.cpp
    src/operations/basic_adjustment_operations/operation_blacks.cpp

    # Engine
    src/engine/photo_engine.cpp
    src/engine/photo_task.cpp
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

# PCH file and optimization
capturemoment_enable_optimizations(capturemoment_core include/pch.h)

# Add headers for IDE
file(GLOB_RECURSE HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
if(HEADER_FILES)
    set_source_files_properties(${HEADER_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)
    target_sources(capturemoment_core PRIVATE ${HEADER_FILES})
endif()

# Link dependencies (already verified by root CMakeLists via PackageHelpers)
# If we reach here, packages are guaranteed to exist
target_link_libraries(capturemoment_core PUBLIC
    OpenImageIO::OpenImageIO
    Halide::Halide
    spdlog::spdlog
    Exiv2::exiv2lib
    magic_enum::magic_enum 
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
