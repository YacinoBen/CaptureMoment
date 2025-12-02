# PackageHelpers.cmake - Helper functions to find and verify required packages

# ============================================================
# Find all required packages
# ============================================================
function(find_required_packages)
    # OpenImageIO (mandatory)
    find_openimageio_package()
    
    # Halide (mandatory)
    find_halide_package()
    
    # Qt6 will be searched by the sub-projects ui/desktop, ui/mobile
endfunction()

# ============================================================
# Find OpenImageIO
# ============================================================
function(find_openimageio_package)
    message(STATUS "🔍 Searching for OpenImageIO...")
    
    # Attempt CONFIG (vcpkg, Conan)
    find_package(OpenImageIO CONFIG QUIET)
    
    if(NOT OpenImageIO_FOUND)
        message(STATUS "⚠️  OpenImageIO not found via CONFIG, trying MODULE...")
        # Attempt MODULE (FindOpenImageIO.cmake)
        find_package(OpenImageIO MODULE QUIET)
    endif()
    
    if(OpenImageIO_FOUND)
        message(STATUS "✅ OpenImageIO found: ${OpenImageIO_VERSION}")
        set(HAVE_OPENIMAGEIO TRUE PARENT_SCOPE)
    else()
        message(FATAL_ERROR "OpenImageIO not found. Please install it via your package manager or vcpkg/conan.")
    endif()
endfunction()

# ============================================================
# Find Halide
# ============================================================
function(find_halide_package)
    message(STATUS "🔍 Searching for Halide...")
    
    find_package(Halide CONFIG QUIET)
    
    if(Halide_FOUND)
        message(STATUS "✅ Halide found: ${Halide_VERSION}")
        set(HAVE_HALIDE TRUE PARENT_SCOPE)
    else()
        message(FATAL_ERROR "Halide not found. Please install it via your package manager or vcpkg/conan.")
    endif()
endfunction()
    message(STATUS "╔════════════════════════════════════════════════════════════╗")
    message(STATUS "║  ⚠️  ATTENTION: Halide requires LLVM                       ║")
    message(STATUS "╠════════════════════════════════════════════════════════════╣")
    message(STATUS "║  If building from source (vcpkg):                         ║")
    message(STATUS "║  - LLVM download: ~50 GB                                  ║")
    message(STATUS "║  - Disk space needed: 100 GB                              ║")
    message(STATUS "║  - RAM required: 16 GB minimum (32 GB recommended)        ║")
    message(STATUS "║  - Build time: 30-120 minutes (depending on hardware)     ║")
    message(STATUS "╠════════════════════════════════════════════════════════════╣")
    message(STATUS "║  💡 RECOMMENDATIONS:                                       ║")
    message(STATUS "║                                                            ║")
    message(STATUS "║  Linux/macOS: Use system package manager                  ║")
    message(STATUS "║    → apt/dnf/brew have prebuilt Halide                    ║")
    message(STATUS "║                                                            ║")
    message(STATUS "║  Windows: Use vcpkg binary cache                          ║")
    message(STATUS "║    → vcpkg install halide --binarysource=...              ║")
    message(STATUS "║    → Or download prebuilt from GitHub Releases            ║")
    message(STATUS "╚════════════════════════════════════════════════════════════╝")
    message(STATUS "")
endfunction()