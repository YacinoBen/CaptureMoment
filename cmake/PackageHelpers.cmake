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
    message(STATUS "╚════════════════════════════════════════════════════════════╝")
    message(STATUS "")
endfunction()