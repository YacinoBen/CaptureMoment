# qt/core/CMakeLists.txt

project(CaptureMomentQtCore LANGUAGES CXX)

message(STATUS "Configuring CaptureMoment Qt Core Library...")

find_package(Qt6 REQUIRED COMPONENTS Core Quick Gui GuiPrivate)

if(TARGET Qt6::Core AND TARGET Qt6::Quick AND TARGET Qt6::Gui AND TARGET Qt6::GuiPrivate)
    message(STATUS "Qt6 found (core): ${Qt6_VERSION} (Core, Quick, Gui, Gui Private)")
else()
    message(FATAL_ERROR "Required Qt6 components (Core, Quick, Gui, Gui Private) not found.")
endif()

qt_standard_project_setup(REQUIRES 6.9.3)

set(CAPTUREMOMENT_QT_CORE_LIB capturemoment_qt_core_lib)

# --- Creation of the library ---
qt_add_library(${CAPTUREMOMENT_QT_CORE_LIB} STATIC)

target_sources(${CAPTUREMOMENT_QT_CORE_LIB}
    PRIVATE
        src/rendering/rhi_image_node.cpp
        src/image_controller.cpp

        # Operations
        src/models/operations/brightness_model.cpp


    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        capturemoment/qt/core/rendering/rhi_image_node.h
        capturemoment/qt/core/image_controller.h

        #Operations
        capturemoment/qt/core/domain/operation_parameters.h
        capturemoment/qt/core/models/operations/i_operation_model.h
        capturemoment/qt/core/models/operations/brightness_model.h
)

# --- Configuration---
target_link_libraries(${CAPTUREMOMENT_QT_CORE_LIB}
    PUBLIC
        Qt6::Core
        Qt6::Quick
        Qt6::Gui 
    PRIVATE
        Qt6::GuiPrivate
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
