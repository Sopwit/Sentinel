cmake_minimum_required(VERSION 3.24)

project(Sentinel
    VERSION 1.0.0
    DESCRIPTION "Sentinel local-first AI desktop assistant"
    LANGUAGES CXX
)

set(SENTINEL_APP_ID "dev.sentinel.Sentinel" CACHE STRING "Reverse-DNS application id")
set(SENTINEL_DISPLAY_NAME "Sentinel Desktop" CACHE STRING "User-visible application name")
set(SENTINEL_ORGANIZATION_NAME "Sentinel" CACHE STRING "Application organization name")
set(SENTINEL_APP_VERSION "1.0.0-rc.1" CACHE STRING "User-visible application version")
set(SENTINEL_BUILD_NUMBER "0" CACHE STRING "Release build number")
set(SENTINEL_COPYRIGHT "Copyright 2026 Sentinel contributors" CACHE STRING "Application copyright")
set(SENTINEL_MACOS_CATEGORY "public.app-category.productivity" CACHE STRING "macOS App Store category")
set(SENTINEL_MACOS_MIN_VERSION "12.0" CACHE STRING "Minimum supported macOS version for release bundles")
set(SENTINEL_CONFIGURED_BUILD_TYPE "${CMAKE_BUILD_TYPE}")
if(NOT SENTINEL_CONFIGURED_BUILD_TYPE)
    set(SENTINEL_CONFIGURED_BUILD_TYPE "unknown")
endif()

find_package(Git QUIET)
set(SENTINEL_GIT_COMMIT "unknown")
if(Git_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short=12 HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE SENTINEL_GIT_COMMIT_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(SENTINEL_GIT_COMMIT_OUTPUT)
        set(SENTINEL_GIT_COMMIT "${SENTINEL_GIT_COMMIT_OUTPUT}")
    endif()
endif()

if(APPLE AND NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "${SENTINEL_MACOS_MIN_VERSION}" CACHE STRING "Minimum macOS deployment target")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Quick Qml Sql Test Network Widgets LinguistTools)

qt_standard_project_setup(REQUIRES 6.5)

add_subdirectory(core)
add_subdirectory(apps/sentinel-desktop)

enable_testing()
add_subdirectory(tests)

# ── CPack Packaging ──────────────────────────────────────────────────────────
if(NOT APPLE)
    set(CPACK_PACKAGE_NAME "sentinel-desktop")
    set(CPACK_PACKAGE_VENDOR "Sentinel Team")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Local-first AI desktop assistant")
    set(CPACK_PACKAGE_CONTACT "support@sentinel.dev")
    configure_file(LICENSE LICENSE.txt COPYONLY)
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_BINARY_DIR}/LICENSE.txt")

    if(WIN32)
        set(CPACK_GENERATOR "NSIS;WIX")
        set(CPACK_PACKAGE_INSTALL_DIRECTORY "Sentinel Desktop")
        
        # NSIS (EXE) Settings
        set(CPACK_NSIS_DISPLAY_NAME "Sentinel Desktop")
        set(CPACK_NSIS_PACKAGE_NAME "Sentinel Desktop")
        set(CPACK_NSIS_HELP_LINK "https://sentinel.dev")
        set(CPACK_NSIS_URL_INFO_ABOUT "https://sentinel.dev")
        set(CPACK_NSIS_CONTACT "support@sentinel.dev")
        set(CPACK_NSIS_MODIFY_PATH OFF) # FIX: Avoid "path too long" error and PATH pollution
        
        # NSIS Visuals
        set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/resources/icons/dev.sentinel.Sentinel.ico")
        set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/resources/icons/dev.sentinel.Sentinel.ico")
        set(CPACK_NSIS_MUI_WELCOMEFINISH_BITMAP "${CMAKE_SOURCE_DIR}/resources/windows/installer/nsis_welcome.bmp")
        set(CPACK_NSIS_MUI_HEADERIMAGE_BITMAP "${CMAKE_SOURCE_DIR}/resources/windows/installer/nsis_header.bmp")
        set(CPACK_NSIS_BRANDING_TEXT "Sentinel Desktop Installer")
        set(CPACK_NSIS_MUI_FINISHPAGE_RUN "sentinel-desktop.exe")
        
        # NSIS Shortcuts & Links (using single backslash in output NSIS script)
        set(CPACK_NSIS_MENU_LINKS 
            "sentinel-desktop.exe" "Sentinel Desktop"
            "https://sentinel.dev" "Sentinel Desktop Website"
        )
        set(CPACK_NSIS_CREATE_ICONS_EXTRA "
            CreateShortCut \\\"\$DESKTOP\\Sentinel Desktop.lnk\\\" \\\"\$INSTDIR\\sentinel-desktop.exe\\\"
        ")
        set(CPACK_NSIS_DELETE_ICONS_EXTRA "
            Delete \\\"\$DESKTOP\\Sentinel Desktop.lnk\\\"
        ")

        # WIX (MSI) Settings
        set(CPACK_WIX_PRODUCT_GUID "a1b2c3d4-e5f6-7a8b-9c0d-e1f2a3b4c5d6")
        set(CPACK_WIX_UPGRADE_GUID "b2c3d4e5-f6a7-0b1c-2d3e-4f5a6b7c8d9e")
        set(CPACK_WIX_UI_REF "WixUI_InstallDir") # Premium multi-page dialog sequence
        set(CPACK_WIX_LICENSE_RTF "${CMAKE_SOURCE_DIR}/resources/windows/installer/license.rtf")
        set(CPACK_WIX_PRODUCT_ICON "${CMAKE_SOURCE_DIR}/resources/icons/dev.sentinel.Sentinel.ico")
        set(CPACK_WIX_UI_BANNER "${CMAKE_SOURCE_DIR}/resources/windows/installer/wix_banner.bmp")
        set(CPACK_WIX_UI_DIALOG "${CMAKE_SOURCE_DIR}/resources/windows/installer/wix_dialog.bmp")
        set(CPACK_WIX_PROGRAM_MENU_FOLDER "Sentinel Desktop")
        set(CPACK_WIX_PROPERTY_ARPHELPLINK "https://sentinel.dev")
        set(CPACK_WIX_PROPERTY_ARPURLINFOABOUT "https://sentinel.dev")
        set(CPACK_WIX_PROPERTY_ARPCONTACT "support@sentinel.dev")

        # Windows Code Signing Integration (removes 'Publisher Unknown' warnings)
        set(SENTINEL_SIGN_THUMBPRINT "" CACHE STRING "SHA-1 Thumbprint of code signing certificate in Windows Certificate Store")
        set(SENTINEL_SIGN_PFX_FILE "" CACHE STRING "Path to the code signing certificate PFX file")
        set(SENTINEL_SIGN_PFX_PASSWORD "" CACHE STRING "Password for the PFX certificate file")
        set(SENTINEL_TIMESTAMP_URL "http://timestamp.digicert.com" CACHE STRING "Timestamp server URL")

        find_program(SIGNTOOL_EXECUTABLE signtool
            HINTS 
                "C:/Program Files (x86)/Windows Kits/10/bin/x64"
                "C:/Program Files (x86)/Windows Kits/10/bin/10.0.22621.0/x64"
                "C:/Program Files (x86)/Windows Kits/10/bin/10.0.19041.0/x64"
        )

        if(SIGNTOOL_EXECUTABLE)
            # Setup command arguments for signtool
            set(SIGN_ARGS sign /fd SHA256 /tr "${SENTINEL_TIMESTAMP_URL}" /td SHA256)
            set(CAN_SIGN FALSE)
            if(SENTINEL_SIGN_THUMBPRINT)
                list(APPEND SIGN_ARGS /sha1 "${SENTINEL_SIGN_THUMBPRINT}")
                set(CAN_SIGN TRUE)
            elseif(SENTINEL_SIGN_PFX_FILE)
                list(APPEND SIGN_ARGS /f "${SENTINEL_SIGN_PFX_FILE}")
                if(SENTINEL_SIGN_PFX_PASSWORD)
                    list(APPEND SIGN_ARGS /p "${SENTINEL_SIGN_PFX_PASSWORD}")
                endif()
                set(CAN_SIGN TRUE)
            endif()

            if(CAN_SIGN)
                message(STATUS "Windows code signing is configured. Installers and binaries will be signed.")
                
                # Sign the main executable after it is built (so it is signed inside the installer)
                add_custom_command(TARGET sentinel-desktop POST_BUILD
                    COMMAND "${SIGNTOOL_EXECUTABLE}" ${SIGN_ARGS} "$<TARGET_FILE:sentinel-desktop>"
                    COMMENT "Signing executable sentinel-desktop.exe to prevent SmartScreen errors"
                )

                # Custom target to sign the generated installer packages after packaging
                add_custom_target(sign_packages
                    COMMAND ${CMAKE_COMMAND} -E echo "Signing generated CPack packages..."
                    COMMAND "${SIGNTOOL_EXECUTABLE}" ${SIGN_ARGS} "${CMAKE_BINARY_DIR}/sentinel-desktop-*.exe"
                    COMMAND "${SIGNTOOL_EXECUTABLE}" ${SIGN_ARGS} "${CMAKE_BINARY_DIR}/sentinel-desktop-*.msi"
                    COMMENT "Signing NSIS and WiX installer files"
                )
            else()
                message(WARNING "signtool found but no certificate thumbprint or PFX file provided. Installers will be unsigned (Publisher: Unknown).")
            endif()
        else()
            message(STATUS "signtool.exe not found. Automatic code signing will be bypassed.")
        endif()
    elseif(UNIX AND NOT APPLE)
        set(CPACK_GENERATOR "DEB;RPM")
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libgl1, libqt6core6, libqt6gui6, libqt6qml6, libqt6quick6, libqt6widgets6, libqt6network6, libqt6sql6")
        set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Sentinel Team <support@sentinel.dev>")
        set(CPACK_RPM_PACKAGE_REQUIRES "qt6-qtbase, qt6-qtdeclarative")
    endif()

    include(CPack)
endif()

