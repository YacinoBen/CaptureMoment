# ============================================================
# BuildOptimization.cmake
# Build optimizations: Unity Build, LTO, PCH
# ============================================================

include_guard(GLOBAL)

# ============================================================
# Function: capturemoment_enable_optimizations
# Description: Applies performance optimizations to a specific target.
#              DO NOT call this on INTERFACE libraries or 3rd party targets.
#
# Arguments:
#   TARGET_NAME  : The name of the target to optimize.
#   PCH_HEADER   : (Optional) Path to a Precompiled Header file.
# ============================================================
function(capturemoment_enable_optimizations TARGET_NAME)

    # Check if target exists to avoid errors
    if(NOT TARGET ${TARGET_NAME})
        message(WARNING "[BuildOptimization] Target '${TARGET_NAME}' does not exist. Skipping.")
        return()
    endif()

    get_target_property(TARGET_TYPE ${TARGET_NAME} TYPE)
    
    # 1. Unity Build Configuration
    # ---------------------------
    # Compiles multiple .cpp files together to reduce overhead.
    # Requires CMake 3.16+
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.16)
        set_target_properties(${TARGET_NAME} PROPERTIES
            UNITY_BUILD ON
            UNITY_BUILD_MODE BATCH
            UNITY_BUILD_BATCH_SIZE 10
        )
        
        # Exclude Qt generated files to prevent ODR conflicts
        if(TARGET_TYPE STREQUAL "STATIC_LIBRARY" OR TARGET_TYPE STREQUAL "EXECUTABLE")
            set_target_properties(${TARGET_NAME} PROPERTIES
                UNITY_BUILD_EXCLUDES "moc_*;qrc_*;qt_*_metatypes"
            )
        endif()

        message(STATUS "[BuildOptimization] Unity Build enabled for target: ${TARGET_NAME}")
    endif()

    # 2. Interprocedural Optimization (IPO / LTO)
    # --------------------------------------------
    # Optimizes across object files. Very effective for static libraries.
    # Only enabled for Release builds to keep Debug fast.
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        include(CheckIPOSupported)
        check_ipo_supported(RESULT ipo_result OUTPUT ipo_output)
        
        if(ipo_result)
            set_target_properties(${TARGET_NAME} PROPERTIES
                INTERPROCEDURAL_OPTIMIZATION TRUE
            )
            message(STATUS "[BuildOptimization] LTO/IPO enabled for target: ${TARGET_NAME}")
        else()
            message(STATUS "[BuildOptimization] IPO not supported for target: ${TARGET_NAME} (${ipo_output})")
        endif()
    endif()

    # 3. Precompiled Headers
    # ----------------------------------
    # If a PCH header path is passed as the second argument.
    if(ARGC GREATER 1)
        set(PCH_FILE "${ARGV1}")
        
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${PCH_FILE}")
            target_precompile_headers(${TARGET_NAME} PRIVATE ${PCH_FILE})
            message(STATUS "[BuildOptimization] Precompiled Header enabled: ${PCH_FILE}")
        else()
            message(WARNING "[BuildOptimization] PCH file not found: ${PCH_FILE}")
        endif()
    endif()

endfunction()
