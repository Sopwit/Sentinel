# cmake/SentinelCPack.cmake
# Cross-Platform CPack Configuration for Sentinel (Windows, Linux, macOS)

set(CPACK_PACKAGE_NAME "sentinel-desktop")
set(CPACK_PACKAGE_VENDOR "Sopwit")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Local-first AI desktop assistant")
set(CPACK_PACKAGE_CONTACT "sopwith.osdev@gmail.com")

configure_file("${CMAKE_SOURCE_DIR}/LICENSE" "${CMAKE_BINARY_DIR}/LICENSE.txt" COPYONLY)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_BINARY_DIR}/LICENSE.txt")

if(WIN32)
    set(CPACK_GENERATOR "NSIS;WIX")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "Sentinel Desktop")
    set(CPACK_PACKAGE_EXECUTABLES "sentinel-desktop" "Sentinel Desktop")
    set(CPACK_CREATE_DESKTOP_LINKS "sentinel-desktop")

    # NSIS (EXE) Settings
    set(CPACK_NSIS_DISPLAY_NAME "Sentinel Desktop")
    set(CPACK_NSIS_PACKAGE_NAME "Sentinel Desktop")
    set(CPACK_NSIS_HELP_LINK "https://sentinel.dev")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://sentinel.dev")
    set(CPACK_NSIS_CONTACT "sopwith.osdev@gmail.com")
    set(CPACK_NSIS_MODIFY_PATH OFF)

    # NSIS Visuals
    set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/resources/app-icons/dev.sentinel.Sentinel.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/resources/app-icons/dev.sentinel.Sentinel.ico")
    set(CPACK_NSIS_MUI_WELCOMEFINISH_BITMAP "${CMAKE_SOURCE_DIR}/resources/platform/windows/installer/nsis_welcome.bmp")
    set(CPACK_NSIS_MUI_HEADERIMAGE_BITMAP "${CMAKE_SOURCE_DIR}/resources/platform/windows/installer/nsis_header.bmp")
    set(CPACK_NSIS_BRANDING_TEXT "Sentinel Desktop Installer")
    set(CPACK_NSIS_MUI_FINISHPAGE_RUN "sentinel-desktop.exe")

    # NSIS Shortcuts & Links (start menu entry created by CPack via CPACK_PACKAGE_EXECUTABLES)
    set(CPACK_NSIS_MENU_LINKS
        "sentinel-desktop.exe" "Sentinel Desktop"
        "https://sentinel.dev" "Sentinel Desktop Website"
    )

    # WiX (MSI) Settings with Parametric GUIDs
    # WARNING: These GUIDs must be regenerated for each release.
    # Use `uuidgen` to generate new ones before packaging.
    set(CPACK_WIX_PRODUCT_GUID "a1b2c3d4-e5f6-7a8b-9c0d-e1f2a3b4c5d6" CACHE STRING "WiX Product GUID — REGENERATE BEFORE RELEASE")
    set(CPACK_WIX_UPGRADE_GUID "b2c3d4e5-f6a7-0b1c-2d3e-4f5a6b7c8d9e" CACHE STRING "WiX Upgrade GUID — REGENERATE BEFORE RELEASE")
    set(CPACK_WIX_UI_REF "WixUI_InstallDir")
    set(CPACK_WIX_LICENSE_RTF "${CMAKE_SOURCE_DIR}/resources/platform/windows/installer/license.rtf")
    set(CPACK_WIX_PRODUCT_ICON "${CMAKE_SOURCE_DIR}/resources/app-icons/dev.sentinel.Sentinel.ico")
    set(CPACK_WIX_UI_BANNER "${CMAKE_SOURCE_DIR}/resources/platform/windows/installer/wix_banner.bmp")
    set(CPACK_WIX_UI_DIALOG "${CMAKE_SOURCE_DIR}/resources/platform/windows/installer/wix_dialog.bmp")
    set(CPACK_WIX_PROGRAM_MENU_FOLDER "Sentinel Desktop")
    set(CPACK_WIX_PROPERTY_ARPHELPLINK "https://sentinel.dev")
    set(CPACK_WIX_PROPERTY_ARPURLINFOABOUT "https://sentinel.dev")
    set(CPACK_WIX_PROPERTY_ARPCONTACT "sopwith.osdev@gmail.com")

elseif(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "DEB;RPM;TGZ")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libgl1, libqt6core6, libqt6gui6, libqt6qml6, libqt6quick6, libqt6widgets6, libqt6network6, libqt6sql6, libqt6sql6-sqlite, libqt6multimedia6")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Sentinel Team <sopwith.osdev@gmail.com>")
    set(CPACK_RPM_PACKAGE_REQUIRES "qt6-qtbase >= 6.5.0, qt6-qtdeclarative >= 6.5.0, qt6-qtsql >= 6.5.0, qt6-qtmultimedia >= 6.5.0, hicolor-icon-theme")
    set(CPACK_RPM_PACKAGE_LICENSE "Apache-2.0")
    set(CPACK_RPM_PACKAGE_GROUP "Applications/System")
    set(CPACK_RPM_PACKAGE_URL "https://sentinel.dev")
    set(CPACK_RPM_PACKAGE_RELEASE "1")
    set(CPACK_RPM_PACKAGE_DESCRIPTION "Local-first AI desktop assistant optimized for Fedora KDE Plasma")
    set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "/usr;/usr/bin;/usr/lib;/usr/lib/systemd;/usr/lib/systemd/user;/usr/share;/usr/share/applications;/usr/share/icons;/usr/share/icons/hicolor;/usr/share/icons/hicolor/scalable;/usr/share/icons/hicolor/scalable/apps;/usr/share/icons/hicolor/1024x1024;/usr/share/icons/hicolor/1024x1024/apps;/usr/share/metainfo;/usr/share/dbus-1;/usr/share/dbus-1/services;/etc;/etc/sentinel")

elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop;TGZ")
    set(CPACK_DMG_VOLUME_NAME "Sentinel Desktop")
    set(CPACK_DMG_FORMAT "UDBZ")
    set(CPACK_BUNDLE_NAME "Sentinel Desktop")
    if(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/apps/sentinel-desktop/sentinel-desktop.app/Contents/Info.plist")
        set(CPACK_BUNDLE_PLIST "${CMAKE_CURRENT_BINARY_DIR}/apps/sentinel-desktop/sentinel-desktop.app/Contents/Info.plist")
    elseif(EXISTS "${CMAKE_SOURCE_DIR}/resources/platform/macos/Info.plist.in")
        set(CPACK_BUNDLE_PLIST "${CMAKE_SOURCE_DIR}/resources/platform/macos/Info.plist.in")
    endif()
    set(CPACK_BUNDLE_ICON "${CMAKE_SOURCE_DIR}/resources/app-icons/dev.sentinel.Sentinel.icns")
    set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/resources/app-icons/dev.sentinel.Sentinel.icns")
endif()

include(CPack)
