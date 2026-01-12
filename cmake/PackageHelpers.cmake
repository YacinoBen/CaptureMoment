# PackageHelpers.cmake - Helper functions to find and verify required packages

# ============================================================
# Find all required packages
# ============================================================
function(find_required_packages)
    # spdlog (mandatory)
    find_spdlog_package()

    # OpenImageIO (mandatory)
    find_openimageio_package()
    
    # Halide (mandatory)
    find_halide_package()

    # Exiv2 (mandatory for serialization)
    find_exiv2_package()

    # magic_enum (mandatory)
    find_magic_enum_package()
    
    # Qt6 will be searched by the sub-projects ui/desktop, ui/mobile

    summarize_found_packages()
    warn_halide_requirements()
endfunction()


# ============================================================
# Find spdlog
# ============================================================
function(find_spdlog_package)
    message(STATUS "Searching for spdlog...")

    # Attempt CONFIG (vcpkg, Conan)
    find_package(spdlog CONFIG QUIET)

    if(NOT spdlog_FOUND)
        message(STATUS "spdlog not found via CONFIG, trying MODULE...")
        # Attempt MODULE (Findspdlog.cmake)
        find_package(spdlog MODULE QUIET)
    endif()
    
    if(spdlog_FOUND)
        set(spdlog_FOUND TRUE PARENT_SCOPE)
        set(spdlog_VERSION "${spdlog_VERSION}" PARENT_SCOPE)
    else()
        message(FATAL_ERROR "spdlog not found. Please install it via your package manager or vcpkg/conan.")
    endif()
endfunction()

# ============================================================
# Find OpenImageIO
# ============================================================
function(find_openimageio_package)
    message(STATUS "Searching for OpenImageIO...")
    
    # Attempt CONFIG (vcpkg, Conan)
    find_package(OpenImageIO CONFIG QUIET)
    
    if(NOT OpenImageIO_FOUND)
        message(STATUS "OpenImageIO not found via CONFIG, trying MODULE...")
        # Attempt MODULE (FindOpenImageIO.cmake)
        find_package(OpenImageIO MODULE QUIET)
    endif()
    
    if(OpenImageIO_FOUND)
        set(OpenImageIO_FOUND TRUE PARENT_SCOPE)
        set(OpenImageIO_VERSION "${OpenImageIO_VERSION}" PARENT_SCOPE)
    else()
        message(FATAL_ERROR "OpenImageIO not found. Please install it via your package manager or vcpkg/conan.")
    endif()
endfunction()

# ============================================================
# Find Halide
# ============================================================
function(find_halide_package)
    message(STATUS "Searching for Halide...")
    
    if(HALIDE_DIR)
    # Path to cmake Halide. -DHALIDE_DIR=/path/to/halide
    find_package(Halide CONFIG REQUIRED HINTS "${HALIDE_DIR}" )
    message(STATUS "Halide Found in this dir : "${HALIDE_DIR}"")
    else()
    find_package(Halide CONFIG QUIET)
    endif()
    
    if(Halide_FOUND)        
        set(Halide_FOUND TRUE PARENT_SCOPE)
        set(Halide_VERSION "${Halide_VERSION}" PARENT_SCOPE)
    else()
        message(FATAL_ERROR "Halide not found. Please install it via your package manager or vcpkg/conan.")
    endif()
endfunction()

# ============================================================
# Warning about Halide requirements
# ============================================================
function(warn_halide_requirements)
  message(STATUS "╔════════════════════════════════════════════════════════════╗")
    message(STATUS "║    Warning: Halide requires LLVM                           ║")
    message(STATUS "╠════════════════════════════════════════════════════════════╣")
    message(STATUS "║  If building from source (vcpkg):                          ║")
    message(STATUS "║  - LLVM download: ~50 GB                                   ║")
    message(STATUS "║  - Disk space needed: 100 GB                               ║")
    message(STATUS "║  - RAM required: 16 GB minimum (32 GB recommended)         ║")
    message(STATUS "║  - Build time: 30-120 minutes (depending on hardware)      ║")
    message(STATUS "╠════════════════════════════════════════════════════════════╣")
    message(STATUS "║     RECOMMENDATIONS:                                       ║")
    message(STATUS "║                                                            ║")
    message(STATUS "║  Linux/macOS: Use system package manager                   ║")
    message(STATUS "║    → apt/dnf/brew have prebuilt Halide                     ║")
    message(STATUS "║                                                            ║")
    message(STATUS "║  Windows: Use vcpkg binary cache                           ║")
    message(STATUS "║    → vcpkg install halide --binarysource=...               ║")
    message(STATUS "║    → Or download prebuilt from GitHub Releases             ║")
    message(STATUS "╚════════════════════════════════════════════════════════════╝")
    message(STATUS "")
endfunction()

# ============================================================
# Find magic_enum
# ============================================================
# ============================================================
# Find magic_enum
# ============================================================
function(find_magic_enum_package)
    message(STATUS "Searching for magic_enum...")

    # Attempt CONFIG first (expects magic_enum-config.cmake from vcpkg, Conan, etc.)
    find_package(magic_enum CONFIG QUIET)

    if(NOT magic_enum_FOUND)
        message(STATUS "magic_enum not found via CONFIG, trying MODULE...")
        # Attempt MODULE (expects Findmagic_enum.cmake or checks standard paths)
        find_package(magic_enum MODULE QUIET)
    endif()

    # If still not found, or if magic_enum is header-only without CMake config, try manual search
    if(NOT magic_enum_FOUND)
        message(STATUS "magic_enum not found via CONFIG/MODULE. Attempting manual search for header-only...")
        find_path(MAGIC_ENUM_INCLUDE_DIR
            NAMES magic_enum/magic_enum.hpp # The main header file we are looking for
            PATHS /usr/local/include       # Standard location after manual install (e.g., GitHub Actions step)
                  /usr/include             # Standard system location (e.g., after apt install if headers were there)
                  # Add other potential standard paths if needed
        )

        if(MAGIC_ENUM_INCLUDE_DIR)
            # Create an INTERFACE imported target for header-only library
            add_library(magic_enum::magic_enum INTERFACE IMPORTED)
            target_include_directories(magic_enum::magic_enum INTERFACE ${MAGIC_ENUM_INCLUDE_DIR})
            set(magic_enum_FOUND TRUE)
            message(STATUS "magic_enum found manually: ${MAGIC_ENUM_INCLUDE_DIR}")
        else()
             message(FATAL_ERROR "magic_enum not found via CONFIG, MODULE, or manual search. Check installation.")
        endif()
    endif()

    if(magic_enum_FOUND)
        message(STATUS "magic_enum found and target magic_enum::magic_enum created/configured.")
        # Export the FOUND status to the parent scope
        set(magic_enum_FOUND TRUE PARENT_SCOPE)
        # No need to export the target name if using it explicitly in target_link_libraries
        # The target magic_enum::magic_enum should be available now
    else()
        message(FATAL_ERROR "magic_enum not found. Please ensure it is installed (e.g., via vcpkg, apt install libmagicenum-dev, or manual install).")
    endif()
endfunction()

# ============================================================
# Find Exiv2
# ============================================================
function(find_exiv2_package)
    message(STATUS "Searching for Exiv2...")

    # Attempt CONFIG (This looks for exiv2-config.cmake or Exiv2Config.cmake)
    # It will look in standard locations and also in CMAKE_PREFIX_PATH
    # The documentation suggests using NAMES exiv2
    find_package(exiv2 CONFIG REQUIRED NAMES exiv2)

    # The imported target name according to the documentation is Exiv2::exiv2lib
    if(TARGET Exiv2::exiv2lib)
        message(STATUS "Exiv2 found: ${exiv2_DIR} (version: ${exiv2_VERSION}) - Target: Exiv2::exiv2lib")
        set(Exiv2_FOUND TRUE PARENT_SCOPE)
        set(exiv2_VERSION "${exiv2_VERSION}" PARENT_SCOPE)
    else()
        message(FATAL_ERROR "Exiv2 found but target Exiv2::exiv2lib is not available. Check installation.")
    endif()

endfunction()

# ============================================================
# Summary of all found packages
# ============================================================
function(summarize_found_packages)
    
    message(STATUS "╔════════════════════════════════════════════════════════════╗")
    message(STATUS "║    Summary Package Information                             ║")
    message(STATUS "╠════════════════════════════════════════════════════════════╣")
    if(spdlog_FOUND)
        message(STATUS "║ spdlog : ${spdlog_VERSION}")
    else()
        message(STATUS "║ spdlog : Not Found")
    endif()

    if(OpenImageIO_FOUND)
        message(STATUS "║ OpenImageIO : ${OpenImageIO_VERSION}")
    else()
        message(STATUS "║ OpenImageIO : Not Found")
    endif()

    if(Halide_FOUND)
        message(STATUS "║ Halide : ${Halide_VERSION}")
    else()
        message(STATUS "║ Halide : Not Found")
    endif()

        if(Halide_FOUND)
        message(STATUS "║ Halide : ${Halide_VERSION}")
    else()
        message(STATUS "║ Halide : Not Found")
    endif()

    if(Exiv2_FOUND)
        message(STATUS "║ Exiv2 : ${exiv2_VERSION}")
    else()
        message(STATUS "║ Exiv2 : Not Found")
    endif()

    if(magic_enum_FOUND)
        message(STATUS "║ magic_enum : ${magic_enum_VERSION}")
    else()
        message(STATUS "║ magic_enum : Not Found")
    endif()

    message(STATUS "╚════════════════════════════════════════════════════════════╝")
    message(STATUS "")
endfunction()