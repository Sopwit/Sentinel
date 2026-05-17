add_library(sentinel_desktop_viewmodels STATIC
    include/sentinel/desktop/ChatMessageListModel.h
    include/sentinel/desktop/DesktopShellViewModel.h
    src/ChatMessageListModel.cpp
    src/DesktopShellViewModel.cpp
)

target_include_directories(sentinel_desktop_viewmodels
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(sentinel_desktop_viewmodels
    PUBLIC
        sentinel_core
        Qt6::Core
)

qt_add_executable(sentinel-desktop
    main.cpp
)

set(SENTINEL_APP_ICON_PNG ${CMAKE_SOURCE_DIR}/resources/icons/dev.sentinel.Sentinel.png)
set(SENTINEL_APP_ICON_SVG ${CMAKE_SOURCE_DIR}/resources/icons/dev.sentinel.Sentinel.svg)
set(SENTINEL_APP_ICON_ICNS ${CMAKE_SOURCE_DIR}/resources/icons/dev.sentinel.Sentinel.icns)
set(SENTINEL_APP_ICON_ICO ${CMAKE_SOURCE_DIR}/resources/icons/dev.sentinel.Sentinel.ico)
set(SENTINEL_WINDOWS_RESOURCE ${CMAKE_CURRENT_BINARY_DIR}/sentinel-desktop.rc)

set_source_files_properties(${SENTINEL_APP_ICON_PNG}
    PROPERTIES
        QT_RESOURCE_ALIAS icons/dev.sentinel.Sentinel.png
)

qt_add_resources(sentinel-desktop sentinel_desktop_assets
    PREFIX /
    FILES
        ${SENTINEL_APP_ICON_PNG}
)

if(APPLE)
    target_sources(sentinel-desktop PRIVATE ${SENTINEL_APP_ICON_ICNS})
    set_source_files_properties(${SENTINEL_APP_ICON_ICNS}
        PROPERTIES
            MACOSX_PACKAGE_LOCATION Resources
    )
endif()

if(WIN32)
    configure_file(
        ${CMAKE_SOURCE_DIR}/resources/windows/sentinel-desktop.rc.in
        ${SENTINEL_WINDOWS_RESOURCE}
        @ONLY
    )
    target_sources(sentinel-desktop PRIVATE
        ${SENTINEL_WINDOWS_RESOURCE}
    )
endif()

set(SENTINEL_DESKTOP_QML_FILES
    ${CMAKE_SOURCE_DIR}/ui/qml/Main.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/Atmosphere.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/ActiveAgentsPanel.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/CognitionStreamPanel.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/HeaderBar.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/InfoRow.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/SectionTitle.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/SentinelButton.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/SentinelDock.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/SentinelOrb.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/SentinelTelemetry.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/SentinelTextField.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/ShellPanel.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/StatusBar.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/WorkspacePresence.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/theme/SentinelTheme.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/pages/ChatPanel.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/pages/AgentsPage.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/pages/DashboardPage.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/pages/MemoryPage.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/pages/SettingsPage.qml
)

set_source_files_properties(${CMAKE_SOURCE_DIR}/ui/qml/Main.qml PROPERTIES QT_RESOURCE_ALIAS Main.qml)
set_source_files_properties(${CMAKE_SOURCE_DIR}/ui/qml/components/Atmosphere.qml PROPERTIES QT_RESOURCE_ALIAS Atmosphere.qml)
set_source_files_properties(${CMAKE_SOURCE_DIR}/ui/qml/components/ActiveAgentsPanel.qml PROPERTIES QT_RESOURCE_ALIAS ActiveAgentsPanel.qml)
set_source_files_properties(${CMAKE_SOURCE_DIR}/ui/qml/components/CognitionStreamPanel.qml PROPERTIES QT_RESOURCE_ALIAS CognitionStreamPanel.qml)
set_source_files_properties(${CMAKE_SOURCE_DIR}/ui/qml/components/HeaderBar.qml PROPERTIES QT_RESOURCE_ALIAS HeaderBar.qml)
set_source_files_properties(${CMAKE_SOURCE_DIR}/ui/qml/components/InfoRow.qml PROPERTIES QT_RESOURCE_ALIAS InfoRow.qml)
set_source_files_properties(${CMAKE_SOURCE_DIR}/ui/qml/components/SectionTitle.qml PROPERTIES QT_RESOURCE_ALIAS SectionTitle.qml)
set_source_files_properties(${CMAKE_SOURCE_DIR}/ui/qml/components/SentinelButton.qml PROPERTIES QT_RESOURCE_ALIAS SentinelButton.qml)
set_source_files_properties(${CMAKE_SOURCE_DIR}/ui/qml/components/SentinelDock.qml PROPERTIES QT_RESOURCE_ALIAS SentinelDock.qml)
set_source_files_properties(${CMAKE_SOURCE_DIR}/ui/qml/components/SentinelOrb.qml PROPERTIES QT_RESOURCE_ALIAS SentinelOrb.qml)
set_source_files_properties(${CMAKE_SOURCE_DIR}/ui/qml/components/SentinelTelemetry.qml PROPERTIES QT_RESOURCE_ALIAS SentinelTelemetry.qml)
set_source_files_properties(${CMAKE_SOURCE_DIR}/ui/qml/components/SentinelTextField.qml PROPERTIES QT_RESOURCE_ALIAS SentinelTextField.qml)
set_source_files_properties(${CMAKE_SOURCE_DIR}/ui/qml/components/ShellPanel.qml PROPERTIES QT_RESOURCE_ALIAS ShellPanel.qml)
set_source_files_properties(${CMAKE_SOURCE_DIR}/ui/qml/components/StatusBar.qml PROPERTIES QT_RESOURCE_ALIAS StatusBar.qml)
set_source_files_properties(${CMAKE_SOURCE_DIR}/ui/qml/components/WorkspacePresence.qml PROPERTIES QT_RESOURCE_ALIAS WorkspacePresence.qml)
set_source_files_properties(${CMAKE_SOURCE_DIR}/ui/qml/theme/SentinelTheme.qml PROPERTIES QT_RESOURCE_ALIAS SentinelTheme.qml QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(${CMAKE_SOURCE_DIR}/ui/qml/pages/ChatPanel.qml PROPERTIES QT_RESOURCE_ALIAS ChatPanel.qml)
set_source_files_properties(${CMAKE_SOURCE_DIR}/ui/qml/pages/AgentsPage.qml PROPERTIES QT_RESOURCE_ALIAS AgentsPage.qml)
set_source_files_properties(${CMAKE_SOURCE_DIR}/ui/qml/pages/DashboardPage.qml PROPERTIES QT_RESOURCE_ALIAS DashboardPage.qml)
set_source_files_properties(${CMAKE_SOURCE_DIR}/ui/qml/pages/MemoryPage.qml PROPERTIES QT_RESOURCE_ALIAS MemoryPage.qml)
set_source_files_properties(${CMAKE_SOURCE_DIR}/ui/qml/pages/SettingsPage.qml PROPERTIES QT_RESOURCE_ALIAS SettingsPage.qml)

qt_add_qml_module(sentinel-desktop
    URI Sentinel.Desktop
    VERSION 1.0
    QML_FILES
        ${SENTINEL_DESKTOP_QML_FILES}
)

target_link_libraries(sentinel-desktop
    PRIVATE
        sentinel_desktop_viewmodels
        sentinel_core
        Qt6::Core
        Qt6::Gui
        Qt6::Quick
        Qt6::Qml
)

set_target_properties(sentinel-desktop PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "Sentinel Desktop"
    MACOSX_BUNDLE_GUI_IDENTIFIER "dev.sentinel.Sentinel"
    MACOSX_BUNDLE_ICON_FILE "dev.sentinel.Sentinel.icns"
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)

install(TARGETS sentinel-desktop
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(UNIX AND NOT APPLE)
    install(FILES ${CMAKE_SOURCE_DIR}/packaging/linux/dev.sentinel.Sentinel.desktop.in
        RENAME dev.sentinel.Sentinel.desktop
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
    )

    install(FILES ${SENTINEL_APP_ICON_SVG}
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps
    )

    install(FILES ${SENTINEL_APP_ICON_PNG}
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/1024x1024/apps
    )
endif()
