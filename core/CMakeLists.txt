add_library(capturemoment_core STATIC
    # Headers
    include/pixel_format.h
    include/image_region.h
    include/common/types.h

    include/i_source_manager.h
    include/source_manager.h
    include/pipeline_engine.h

    # Operations
    include/operations/i_operation.h
    include/operations/operation_type.h
    include/operations/operation_descriptor.h
    include/operations/operation_factory.h
    include/operations/operation_brightness.h

    # Sources
    src/image_region.cpp
    src/source_manager.cpp
    src/pipeline_engine.cpp

    # Source operations
    src/operations/operation_factory.cpp
    src/operations/operation_brightness.cpp
)

target_link_libraries(capturemoment_core
    PUBLIC
        OpenImageIO::OpenImageIO
        Halide::Halide
)

target_include_directories(capturemoment_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# C++23 requis
target_compile_features(capturemoment_core PUBLIC cxx_std_23)