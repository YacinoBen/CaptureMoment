# CompilerDetection.cmake - Detect and verify compiler capabilities

# global variable to hold detected compiler type
set(DETECTED_COMPILER "Unknown" CACHE INTERNAL "Detected compiler type")

# ============================================================
# Detect compiler
# ============================================================
function(detect_compiler)
    if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        set(DETECTED_COMPILER "MSVC" CACHE INTERNAL "Detected compiler")
        message(STATUS "🔍 Compiler detected: MSVC ${CMAKE_CXX_COMPILER_VERSION}")
        
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        set(DETECTED_COMPILER "MinGW" CACHE INTERNAL "Detected compiler")
        message(STATUS "🔍 Compiler detected: MinGW/GCC ${CMAKE_CXX_COMPILER_VERSION}")
        
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(DETECTED_COMPILER "Clang" CACHE INTERNAL "Detected compiler")
        message(STATUS "🔍 Compiler detected: Clang ${CMAKE_CXX_COMPILER_VERSION}")
        
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
        set(DETECTED_COMPILER "AppleClang" CACHE INTERNAL "Detected compiler")
        message(STATUS "🔍 Compiler detected: Apple Clang ${CMAKE_CXX_COMPILER_VERSION}")
        
    else()
        set(DETECTED_COMPILER "Unknown" CACHE INTERNAL "Detected compiler")
        message(WARNING "⚠️  Compiler not recognized: ${CMAKE_CXX_COMPILER_ID}")
    endif()
endfunction()

# ============================================================
# Verify C++23 support
# ============================================================
function(verify_cpp23_support)
    detect_compiler()
    
    set(MIN_VERSION_MSVC "19.30")      # Visual Studio 2022 17.0+
    set(MIN_VERSION_GCC "13.0")        # GCC 13+
    set(MIN_VERSION_CLANG "16.0")      # Clang 16+
    set(MIN_VERSION_APPLECLANG "15.0") # Xcode 15+
    
    if(DETECTED_COMPILER STREQUAL "MSVC")
        if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS MIN_VERSION_MSVC)
            message(FATAL_ERROR "❌ MSVC ${CMAKE_CXX_COMPILER_VERSION} does not support C++23. Minimum required: ${MIN_VERSION_MSVC}")
        endif()
        
    elseif(DETECTED_COMPILER STREQUAL "MinGW")
        if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS MIN_VERSION_GCC)
            message(FATAL_ERROR "❌ GCC ${CMAKE_CXX_COMPILER_VERSION} does not support C++23. Minimum required: ${MIN_VERSION_GCC}")
        endif()
        
    elseif(DETECTED_COMPILER STREQUAL "Clang")
        if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS MIN_VERSION_CLANG)
            message(FATAL_ERROR "❌ Clang ${CMAKE_CXX_COMPILER_VERSION} does not support C++23. Minimum required: ${MIN_VERSION_CLANG}")
        endif()
        
    elseif(DETECTED_COMPILER STREQUAL "AppleClang")
        if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS MIN_VERSION_APPLECLANG)
            message(FATAL_ERROR "❌ Apple Clang ${CMAKE_CXX_COMPILER_VERSION} does not support C++23. Minimum required: ${MIN_VERSION_APPLECLANG}")
        endif()
    endif()
    
    message(STATUS "✅ C++23 support verified")
endfunction()

# ============================================================
# Get compiler name (user-friendly)
# ============================================================
function(get_compiler_name OUTPUT_VAR)
    detect_compiler()
    
    if(DETECTED_COMPILER STREQUAL "MSVC")
        set(${OUTPUT_VAR} "MSVC ${CMAKE_CXX_COMPILER_VERSION}" PARENT_SCOPE)
    elseif(DETECTED_COMPILER STREQUAL "MinGW")
        set(${OUTPUT_VAR} "MinGW/GCC ${CMAKE_CXX_COMPILER_VERSION}" PARENT_SCOPE)
    elseif(DETECTED_COMPILER STREQUAL "Clang")
        set(${OUTPUT_VAR} "Clang ${CMAKE_CXX_COMPILER_VERSION}" PARENT_SCOPE)
    elseif(DETECTED_COMPILER STREQUAL "AppleClang")
        set(${OUTPUT_VAR} "Apple Clang ${CMAKE_CXX_COMPILER_VERSION}" PARENT_SCOPE)
    else()
        set(${OUTPUT_VAR} "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}" PARENT_SCOPE)
    endif()
endfunction()