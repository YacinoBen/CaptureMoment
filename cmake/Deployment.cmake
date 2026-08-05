# cmake/Deployment.cmake

# Skip packaging if no UI is built
if(NOT BUILD_DESKTOP_UI AND NOT BUILD_MOBILE_UI)
    return()
endif()

# ============================================================
# Desktop Packaging (CPack)
# ============================================================
if(BUILD_DESKTOP_UI)

    set(CPACK_PACKAGE_NAME "CaptureMoment")
    set(CPACK_PACKAGE_VENDOR "CaptureMoment Team")
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
    set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
    set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
    set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

    # Platform specific generators
    if(WIN32)
        set(CPACK_GENERATOR "NSIS")
        
        # NSIS specific settings
        set(CPACK_NSIS_DISPLAY_NAME "Capture Moment")
        set(CPACK_NSIS_PACKAGE_NAME "CaptureMoment")
        set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
        set(CPACK_PACKAGE_INSTALL_DIRECTORY "CaptureMoment")
        
        # Set the .ico for the NSIS installer UI itself
        set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/assets/icons/favicon.ico")
        set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/assets/icons/favicon.ico")
        
        # Create desktop shortcut using the exact target name (capturemoment_desktop.exe)
        set(CPACK_NSIS_CREATE_ICONS_EXTRA 
            "CreateShortCut '$DESKTOP\\Capture Moment.lnk' '$INSTDIR\\bin\\capturemoment_desktop.exe'"
        )
        set(CPACK_NSIS_DELETE_ICONS_EXTRA 
            "Delete '$DESKTOP\\Capture Moment.lnk'"
        )

    elseif(APPLE)
        set(CPACK_GENERATOR "DragNDrop")
        
        # The .icns is already handled by MACOSX_BUNDLE_ICON_FILE in qt/desktop/CMakeLists.txt
        set(CPACK_DMG_VOLUME_NAME "Capture Moment")

    elseif(UNIX)
        set(CPACK_GENERATOR "DEB;RPM")
        
        # The .png for hicolor is already handled in qt/desktop/CMakeLists.txt
        set(CPACK_PACKAGE_CONTACT "CaptureMoment Team")
        set(CPACK_DEBIAN_PACKAGE_SECTION "graphics")
        set(CPACK_RPM_PACKAGE_GROUP "Applications/Multimedia")
    endif()

    # Include CPack module
    include(CPack)
endif()

# ============================================================
# Mobile Packaging (Future - Android / iOS)
# ============================================================
if(BUILD_MOBILE_UI)
    # Mobile packaging is handled by Qt's dedicated deployment tools:
    # - Android: Gradle integration via qt_add_executable() -> generates APK/AAB
    # - iOS: Xcode archiving -> generates IPA
    
    # Future mobile-specific post-build steps (if needed) will go here.
endif()