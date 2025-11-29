cmake_minimum_required(VERSION 3.21)

project(CaptureMoment VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compilation options
option(BUILD_DESKTOP_UI "Build desktop UI" ON)
option(BUILD_MOBILE_UI "Build mobile UI" OFF) # On after
option(BUILD_TESTS "Build tests" OFF) # On after

# Add subprojects
if(BUILD_DESKTOP_UI)
    add_subdirectory(ui/desktop)
endif()

if(BUILD_MOBILE_UI)
    add_subdirectory(ui/mobile)
endif()

# core/, catalog/, color/, etc. Later :
# add_subdirectory(core)
# add_subdirectory(catalog)
# add_subdirectory(color)
# add_subdirectory(tests)
