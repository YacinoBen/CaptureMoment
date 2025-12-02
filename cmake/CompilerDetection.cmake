# CompilerDetection.cmake - Optimized compiler detection

# ============================================================
# Compiler requirements (C++23)
# ============================================================
# Format: COMPILER_ID|MIN_VERSION|NAME
set(SUPPORTED_COMPILERS
    "MSVC|19.30|MSVC (Visual Studio 2022 17.0+)"
    "GNU|13.0|GCC"
    "Clang|16.0|Clang"
    "AppleClang|15.0|Apple Clang (Xcode 15+)"
    "Intel|2023.0|Intel oneAPI"
    "IntelLLVM|2023.0|Intel LLVM"
    "ARMClang|16.0|ARM Clang"
)

function(verify_cpp23_support)
    set(COMPILER_FOUND FALSE)
    
    foreach(ENTRY ${SUPPORTED_COMPILERS})
        # Split the entry using the pipe '|' delimiter into a CMake list.
        string(REPLACE "|" ";" ENTRY_LIST "${ENTRY}")
        
        list(GET ENTRY_LIST 0 COMPILER_ID)
        list(GET ENTRY_LIST 1 MIN_VERSION)
        list(GET ENTRY_LIST 2 NAME)
        
        if(CMAKE_CXX_COMPILER_ID STREQUAL "${COMPILER_ID}") # STREQUAL is safer than MATCHES for exact ID check
            set(COMPILER_FOUND TRUE)
            
            if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS MIN_VERSION)
                message(FATAL_ERROR 
                    "${NAME} ${CMAKE_CXX_COMPILER_VERSION} < ${MIN_VERSION} (required for C++23).")
            endif()
            
            message(STATUS "C++23 support verified (${NAME} ${CMAKE_CXX_COMPILER_VERSION})")
            return()
        endif()
    endforeach()
    
    if(NOT COMPILER_FOUND)
        message(WARNING "Compiler '${CMAKE_CXX_COMPILER_ID}' not officially tested. Build may fail.")
    endif()
endfunction()

function(get_compiler_name OUTPUT_VAR)
    set(RESULT "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
    
    foreach(ENTRY ${SUPPORTED_COMPILERS})
        string(REPLACE "|" ";" ENTRY_LIST "${ENTRY}")
        
        list(GET ENTRY_LIST 0 COMPILER_ID)
        list(GET ENTRY_LIST 2 NAME)
        
        if(CMAKE_CXX_COMPILER_ID STREQUAL "${COMPILER_ID}")
            set(RESULT "${NAME} ${CMAKE_CXX_COMPILER_VERSION}")
            break()
        endif()
    endforeach()
    
    set(${OUTPUT_VAR} "${RESULT}" PARENT_SCOPE)
endfunction()