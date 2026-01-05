# qt/core/CMakeLists.txt

project(CaptureMomentQtCore LANGUAGES CXX)

message(STATUS "Configuring CaptureMoment Qt Core Library...")

find_package(Qt6 REQUIRED COMPONENTS Core Quick Gui GuiPrivate ShaderTools)

if(TARGET Qt6::Core)
    message(STATUS "Qt6 found Core")
else()
    message(FATAL_ERROR "Core not found.")
endif()

if(TARGET Qt6::Quick)
    message(STATUS "Qt6 found Quick")
else()
    message(FATAL_ERROR "Quick not found.")
endif()

if(TARGET Qt6::Gui)
    message(STATUS "Qt6 found Gui")
else()
    message(FATAL_ERROR "Gui not found.")
endif()

if(TARGET Qt6::GuiPrivate)
    message(STATUS "Qt6 found Gui Private")
else()
    message(FATAL_ERROR "Gui Private not found.")
endif()

if(TARGET Qt6::ShaderTools)
    message(STATUS "Qt6 found ShaderTools")
else()
    message(FATAL_ERROR "ShaderTools not found.")
endif()

message(STATUS "Qt6 found : ${Qt6_VERSION}")

qt_standard_project_setup(REQUIRES 6.9.3)

set(CAPTUREMOMENT_QT_CORE_LIB capturemoment_qt_core_lib)

# --- Creation of the library ---
qt_add_library(${CAPTUREMOMENT_QT_CORE_LIB} STATIC)

# Explicitly ensure AUTOMOC runs on this target
set_target_properties(${CAPTUREMOMENT_QT_CORE_LIB} PROPERTIES
    AUTOMOC TRUE
    AUTORCC TRUE
    AUTOUIC TRUE
)

qt_add_shaders(${CAPTUREMOMENT_QT_CORE_LIB}
    PREFIX /shaders
    FILES
        shaders/glsl/image_display.vert
        shaders/glsl/image_display.frag
)

set(CORE_SOURCES
    # Controller
    src/controller/image_controller_base.cpp
    src/controller/image_controller_painted.cpp
    src/controller/image_controller_sgs.cpp
    src/controller/image_controller_rhi.cpp

    # Display
    src/display/display_manager.cpp

    # Rendering
    src/rendering/rhi_image_node.cpp
    src/rendering/rhi_image_item.cpp
    src/rendering/rhi_image_item_renderer.cpp
    src/rendering/sgs_image_item.cpp
    src/rendering/painted_image_item.cpp
    src/rendering/base_image_item.cpp

    # Operations base adjustment
    src/models/operations/base_adjustment_model.cpp
    src/models/operations/brightness_model.cpp
    src/models/operations/contrast_model.cpp
    src/models/operations/highlights_model.cpp
    src/models/operations/shadows_model.cpp
    src/models/operations/whites_model.cpp
    src/models/operations/blacks_model.cpp
)

set(QOBJECT_HEADERS
    # Rendering
    include/rendering/i_rendering_item_base.h
    include/rendering/rhi_image_node.h
    include/rendering/rhi_image_item.h
    include/rendering/rhi_image_item_renderer.h
    include/rendering/sgs_image_item.h
    include/rendering/painted_image_item.h
    include/rendering/base_image_item.h

    # Display
    include/display/display_manager.h

    # Controller
    include/controller/image_controller_base.h
    include/controller/image_controller_painted.h
    include/controller/image_controller_sgs.h
    include/controller/image_controller_rhi.h

    # Operations
    include/models/operations/i_operation_model.h
    include/models/operations/operation_provider.h

    # List Operations base adjustment
    include/models/operations/base_adjustment_model.h
    include/models/operations/brightness_model.h
    include/models/operations/contrast_model.h
    include/models/operations/highlights_model.h
    include/models/operations/shadows_model.h
    include/models/operations/whites_model.h
    include/models/operations/blacks_model.h
)

target_sources(${CAPTUREMOMENT_QT_CORE_LIB}
    PRIVATE
        ${CORE_SOURCES} 
        ${QOBJECT_HEADERS}

    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
            # Controller
            controller/image_controller_base.h
            controller/image_controller_painted.h
            controller/image_controller_sgs.h
            controller/image_controller_rhi.h

            # Display
            display/display_manager.h

            # Rendering
            rendering/i_rendering_item_base.h
            rendering/base_image_item.h
            rendering/rhi_image_node.h
            rendering/rhi_image_item.h
            rendering/rhi_image_item_renderer.h
            rendering/sgs_image_item.h
            rendering/painted_image_item.h

            # Operations
            domain/operation_parameters.h
            models/operations/i_operation_model.h
            models/operations/operation_provider.h

            # List Operations base adjustment
            models/operations/base_adjustment_model.h
            models/operations/brightness_model.h
            models/operations/contrast_model.h
            models/operations/highlights_model.h
            models/operations/shadows_model.h
            models/operations/whites_model.h
            models/operations/blacks_model.h
)

# --- Configuration---
target_link_libraries(${CAPTUREMOMENT_QT_CORE_LIB}
    PUBLIC
        Qt6::Core
        Qt6::Quick
        Qt6::Gui
        Qt6::GuiPrivate
        Qt6::ShaderTools
        capturemoment_core        
)

target_include_directories(${CAPTUREMOMENT_QT_CORE_LIB}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> 
        $<INSTALL_INTERFACE:include> 
    PRIVATE
    # If necessary, include other internal paths to this library
    # ${CMAKE_CURRENT_SOURCE_DIR}/src # If the .cpp files need to be included from src/
    # Path to the core headers if needed in the .cpp files of this library
    # "${CMAKE_SOURCE_DIR}/core/include"
)

install(TARGETS ${CAPTUREMOMENT_QT_CORE_LIB}
    EXPORT CaptureMomentQtCoreTargets 
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} 
    FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/capturemoment_qt_core
)


install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/capturemoment_qt_core
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

message(STATUS "CaptureMoment Qt Core Library configured successfully.")
message(STATUS "   - Target: ${CAPTUREMOMENT_QT_CORE_LIB}")
message(STATUS "   - Qt Version: ${Qt6_VERSION}")
message(STATUS "")
