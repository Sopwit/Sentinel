add_library(sentinel_desktop_viewmodels STATIC
    include/sentinel/desktop/ChatMessageListModel.h
    include/sentinel/desktop/DesktopShellViewModel.h
    include/sentinel/desktop/viewmodels/ChatViewModel.h
    src/ChatMessageListModel.cpp
    src/DesktopShellViewModel.cpp
    src/viewmodels/ChatViewModel.cpp
)

add_library(sentinel_desktop_startup STATIC
    include/sentinel/desktop/GraphicsBackend.h
    src/GraphicsBackend.cpp
)

target_include_directories(sentinel_desktop_startup
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(sentinel_desktop_startup
    PUBLIC
        Qt6::Core
        Qt6::Quick
)

target_include_directories(sentinel_desktop_viewmodels
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(sentinel_desktop_viewmodels
    PUBLIC
        sentinel_core
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
)

qt_add_executable(sentinel-desktop
    include/sentinel/desktop/NativeCompanionAdapter.h
    include/sentinel/desktop/DaemonClient.h
    bootstrap/ApplicationBootstrapper.h
    bootstrap/ApplicationBootstrapper.cpp
    bootstrap/GraphicsInitializer.h
    bootstrap/GraphicsInitializer.cpp
    bootstrap/LoggingInitializer.h
    bootstrap/LoggingInitializer.cpp
    bootstrap/PlatformInitializer.h
    bootstrap/PlatformInitializer.cpp
    bootstrap/SingleInstanceGuard.h
    bootstrap/SingleInstanceGuard.cpp
    main.cpp
    src/NativeCompanionAdapter.cpp
    src/DaemonClient.cpp
)

set(SENTINEL_TRANSLATION_SOURCES
    ${CMAKE_SOURCE_DIR}/translations/sentinel_en.ts
    ${CMAKE_SOURCE_DIR}/translations/sentinel_tr.ts
    ${CMAKE_SOURCE_DIR}/translations/sentinel_de.ts
    ${CMAKE_SOURCE_DIR}/translations/sentinel_es.ts
    ${CMAKE_SOURCE_DIR}/translations/sentinel_fr.ts
    ${CMAKE_SOURCE_DIR}/translations/sentinel_zh.ts
    ${CMAKE_SOURCE_DIR}/translations/sentinel_ja.ts
    ${CMAKE_SOURCE_DIR}/translations/sentinel_ar.ts
)

qt_add_translations(sentinel-desktop
    TS_FILES ${SENTINEL_TRANSLATION_SOURCES}
)

add_custom_target(update_translations
    COMMAND Qt6::lupdate ${CMAKE_SOURCE_DIR}/core ${CMAKE_SOURCE_DIR}/apps/sentinel-desktop ${CMAKE_SOURCE_DIR}/ui/qml -ts ${SENTINEL_TRANSLATION_SOURCES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Updating translation .ts files with Qt lupdate"
)

set(SENTINEL_APP_ICON_PNG ${CMAKE_SOURCE_DIR}/resources/app-icons/dev.sentinel.Sentinel.png)
set(SENTINEL_APP_ICON_SVG ${CMAKE_SOURCE_DIR}/resources/app-icons/dev.sentinel.Sentinel.svg)
set(SENTINEL_APP_ICON_ICNS ${CMAKE_SOURCE_DIR}/resources/app-icons/dev.sentinel.Sentinel.icns)
set(SENTINEL_APP_ICON_ICO ${CMAKE_SOURCE_DIR}/resources/app-icons/dev.sentinel.Sentinel.ico)
set(SENTINEL_WINDOWS_MANIFEST ${CMAKE_SOURCE_DIR}/resources/platform/windows/sentinel-desktop.exe.manifest)
set(SENTINEL_WINDOWS_RESOURCE ${CMAKE_CURRENT_BINARY_DIR}/sentinel-desktop.rc)
set(SENTINEL_MACOS_INFO_PLIST ${CMAKE_SOURCE_DIR}/resources/platform/macos/Info.plist.in)

set_source_files_properties(${SENTINEL_APP_ICON_PNG}
    PROPERTIES
        QT_RESOURCE_ALIAS icons/dev.sentinel.Sentinel.png
)
set_source_files_properties(${CMAKE_SOURCE_DIR}/resources/sounds/notification.wav
    PROPERTIES
        QT_RESOURCE_ALIAS sounds/notification.wav
)
set_source_files_properties(${CMAKE_SOURCE_DIR}/resources/sounds/error.wav
    PROPERTIES
        QT_RESOURCE_ALIAS sounds/error.wav
)
set_source_files_properties(${CMAKE_SOURCE_DIR}/resources/sounds/success.wav
    PROPERTIES
        QT_RESOURCE_ALIAS sounds/success.wav
)

set_source_files_properties(${CMAKE_SOURCE_DIR}/resources/fonts/InterVariable.ttf
    PROPERTIES
        QT_RESOURCE_ALIAS fonts/Inter/Inter-Variable.ttf
)
set_source_files_properties(${CMAKE_SOURCE_DIR}/resources/fonts/IBMPlexMono-Regular.ttf
    PROPERTIES
        QT_RESOURCE_ALIAS fonts/IBM_Plex_Mono/IBMPlexMono-Regular.ttf
)

qt_add_resources(sentinel-desktop sentinel_desktop_assets
    PREFIX /
    FILES
        ${SENTINEL_APP_ICON_PNG}
        ${CMAKE_SOURCE_DIR}/resources/sounds/notification.wav
        ${CMAKE_SOURCE_DIR}/resources/sounds/error.wav
        ${CMAKE_SOURCE_DIR}/resources/sounds/success.wav
        ${CMAKE_SOURCE_DIR}/resources/fonts/InterVariable.ttf
        ${CMAKE_SOURCE_DIR}/resources/fonts/IBMPlexMono-Regular.ttf
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
        ${CMAKE_SOURCE_DIR}/resources/platform/windows/sentinel-desktop.rc.in
        ${SENTINEL_WINDOWS_RESOURCE}
        @ONLY
    )
    target_sources(sentinel-desktop PRIVATE
        ${SENTINEL_WINDOWS_RESOURCE}
    )
endif()

set(SENTINEL_DESKTOP_QML_FILES
    ${CMAKE_SOURCE_DIR}/ui/qml/Main.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/theme/SentinelTheme.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/theme/MotionTokens.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/theme/InteractionTokens.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/base/Atmosphere.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/base/EmptyState.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/base/ErrorBanner.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/base/GlowSurface.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/base/InfoRow.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/base/RuntimeBadge.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/base/SectionTitle.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/base/SentinelButton.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/base/SentinelComboBox.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/base/SentinelOrb.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/base/SentinelSlider.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/base/SentinelSpinBox.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/base/SentinelSwitch.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/base/SentinelTelemetry.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/base/SentinelTextField.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/base/SettingCard.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/base/SettingControlRow.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/base/SettingToggleRow.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/base/ShimmerEffect.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/base/SoundManager.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/base/StatusChip.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/base/StatusPulse.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/chat/HomeChatSurface.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/chat/ActiveAgentsPanel.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/chat/CognitionStreamPanel.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/dialogs/ModelDetailPopup.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/dialogs/RuntimeDetailPopup.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/dialogs/SentinelOverlayModal.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/dialogs/SplashScreen.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/dialogs/TrayCompanionWindow.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/dialogs/UpdateProgressModal.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/navigation/BottomDock.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/navigation/CommandPalette.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/navigation/HeaderBar.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/navigation/ShellPanel.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/navigation/StatusBar.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/navigation/TitleBar.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/navigation/TitleBarButton.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/notifications/NotificationCenterPanel.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/notifications/NotificationItemDelegate.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/components/notifications/NotificationToast.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/pages/ChatPanel.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/pages/DashboardPage.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/pages/models/ModelsPage.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/pages/models/LocalModelsTab.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/pages/models/OnlineRegistryTab.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/pages/settings/SettingsPage.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/pages/settings/AppearanceSettingsTab.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/pages/settings/ModelSettingsTab.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/pages/settings/VoiceSettingsTab.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/pages/settings/WorkspaceSettingsTab.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/pages/settings/SecuritySettingsTab.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/pages/settings/SystemSettingsTab.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/onboarding/OnboardingScreen.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/onboarding/OnboardingToggle.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/onboarding/WelcomeStep.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/onboarding/PrivacyConsentStep.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/onboarding/AppearanceStep.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/onboarding/ProviderSetupStep.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/onboarding/ModelSetupStep.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/onboarding/PreferencesStep.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/onboarding/VoiceSetupStep.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/onboarding/CapabilitiesStep.qml
    ${CMAKE_SOURCE_DIR}/ui/qml/onboarding/FinishStep.qml
)

sentinel_register_qml_files(
    FILES
        ${SENTINEL_DESKTOP_QML_FILES}
    SINGLETONS
        SentinelTheme.qml
        MotionTokens.qml
        InteractionTokens.qml
)

sentinel_apply_compiler_flags(sentinel_desktop_viewmodels)
sentinel_apply_compiler_flags(sentinel_desktop_startup)
sentinel_apply_compiler_flags(sentinel-desktop)

sentinel_enable_pch(sentinel_desktop_viewmodels)
sentinel_enable_pch(sentinel-desktop)


qt_add_qml_module(sentinel-desktop
    URI Sentinel.Desktop
    VERSION 1.0
    QML_FILES
        ${SENTINEL_DESKTOP_QML_FILES}
)

target_link_libraries(sentinel-desktop
    PRIVATE
        sentinel_desktop_viewmodels
        sentinel_desktop_startup
        sentinel_core
        Qt6::Core
        Qt6::Gui
        Qt6::Quick
        Qt6::Qml
        Qt6::Widgets
        Qt6::Multimedia
)

set_target_properties(sentinel-desktop PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "Sentinel Desktop"
    MACOSX_BUNDLE_GUI_IDENTIFIER "dev.sentinel.Sentinel"
    MACOSX_BUNDLE_ICON_FILE "dev.sentinel.Sentinel.icns"
    MACOSX_BUNDLE_INFO_PLIST ${SENTINEL_MACOS_INFO_PLIST}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${SENTINEL_BUILD_NUMBER}
    WIN32_EXECUTABLE TRUE
)

if(APPLE)
    set(SENTINEL_MACOS_RESOURCES
        ${SENTINEL_APP_ICON_ICNS}
        ${CMAKE_SOURCE_DIR}/resources/platform/macos/PrivacyInfo.xcprivacy
        ${CMAKE_SOURCE_DIR}/resources/platform/macos/Sentinel.sdef
        ${CMAKE_SOURCE_DIR}/packaging/macos/uninstall.sh
    )
    set_source_files_properties(${SENTINEL_MACOS_RESOURCES} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
    set_source_files_properties(${CMAKE_SOURCE_DIR}/resources/platform/macos/en.lproj/InfoPlist.strings PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources/en.lproj"
    )
    set_source_files_properties(${CMAKE_SOURCE_DIR}/resources/platform/macos/tr.lproj/InfoPlist.strings PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources/tr.lproj"
    )
    target_sources(sentinel-desktop PRIVATE
        ${SENTINEL_MACOS_RESOURCES}
        ${CMAKE_SOURCE_DIR}/resources/platform/macos/en.lproj/InfoPlist.strings
        ${CMAKE_SOURCE_DIR}/resources/platform/macos/tr.lproj/InfoPlist.strings
    )
    set_target_properties(sentinel-desktop PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_SOURCE_DIR}/resources/platform/macos/sentinel.entitlements"
    )
endif()

if(MINGW)
    target_compile_options(sentinel-desktop PRIVATE "-Wa,-mbig-obj")
endif()

include(GNUInstallDirs)

set(SENTINEL_INSTALL_DIR "${CMAKE_INSTALL_BINDIR}")
if(WIN32)
    # Native Windows industry standard: place main executable in the installation root
    set(SENTINEL_INSTALL_DIR ".")
endif()

install(TARGETS sentinel-desktop
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${SENTINEL_INSTALL_DIR}
)

if(WIN32)
    # Set properties for CPack WiX generator to create Start Menu and Desktop shortcuts natively
    set_target_properties(sentinel-desktop PROPERTIES
        CPACK_DESKTOP_SHORTCUTS "Sentinel Desktop"
        CPACK_START_MENU_SHORTCUTS "Sentinel Desktop"
    )

    get_target_property(_qmake_path Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_path}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

    if(WINDEPLOYQT_EXECUTABLE)
        install(CODE "
            message(STATUS \"Running windeployqt on \${CMAKE_INSTALL_PREFIX}/${SENTINEL_INSTALL_DIR}/sentinel-desktop.exe\")
            execute_process(
                COMMAND \"${WINDEPLOYQT_EXECUTABLE}\" \"\${CMAKE_INSTALL_PREFIX}/${SENTINEL_INSTALL_DIR}/sentinel-desktop.exe\" --qmldir \"${CMAKE_SOURCE_DIR}/ui/qml\" --compiler-runtime
                RESULT_VARIABLE windeployqt_res
            )
            if(NOT windeployqt_res EQUAL 0)
                message(WARNING \"windeployqt failed with code \${windeployqt_res}\")
            endif()
        ")
    else()
        message(WARNING "windeployqt executable not found!")
    endif()

    # Bundle MinGW runtime DLLs (libstdc++-6.dll, libgcc_s_seh-1.dll, libwinpthread-1.dll)
    # These are required on systems without MinGW installed
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        get_filename_component(_mingw_bin_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
        set(_mingw_runtime_dlls
            "${_mingw_bin_dir}/libstdc++-6.dll"
            "${_mingw_bin_dir}/libgcc_s_seh-1.dll"
            "${_mingw_bin_dir}/libwinpthread-1.dll"
        )
        install(FILES ${_mingw_runtime_dlls}
            DESTINATION ${SENTINEL_INSTALL_DIR}
        )
    endif()
endif()

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

    install(FILES ${CMAKE_SOURCE_DIR}/packaging/linux/dev.sentinel.Sentinel.metainfo.xml
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo
    )

    install(FILES ${CMAKE_SOURCE_DIR}/LICENSE
        DESTINATION ${CMAKE_INSTALL_DOCDIR}
    )

    install(FILES ${CMAKE_SOURCE_DIR}/README.md
        DESTINATION ${CMAKE_INSTALL_DOCDIR}
    )

    install(FILES ${CMAKE_SOURCE_DIR}/packaging/etc/sentinel/config.json.template
        DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/sentinel
    )

    install(FILES ${CMAKE_SOURCE_DIR}/packaging/systemd/sentinel-desktop.service
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/systemd/user
    )

    install(FILES ${CMAKE_SOURCE_DIR}/packaging/dbus/dev.sentinel.Sentinel.service
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/dbus-1/services
    )
endif()

