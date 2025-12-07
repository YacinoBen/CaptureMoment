# Examples for CaptureMoment

# Basic Brightness Example
add_executable(basic_brightness
    basic_brightness.cpp
)

target_link_libraries(basic_brightness
    PRIVATE
        capturemoment_core
        spdlog::spdlog
)

target_include_directories(basic_brightness
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

target_compile_features(basic_brightness PRIVATE cxx_std_23)

if(MSVC)
    target_compile_options(basic_brightness PRIVATE /W4 /std:c++latest)
else()
    target_compile_options(basic_brightness PRIVATE -Wall -Wextra -pedantic)
endif()

# Optional: Add more examples later
# add_executable(advanced_pipeline advanced_pipeline.cpp)
# target_link_libraries(advanced_pipeline PRIVATE capturemoment_core)