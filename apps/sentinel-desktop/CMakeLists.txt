add_library(sentinel_desktop_viewmodels STATIC
    include/sentinel/desktop/DesktopShellViewModel.h
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

set(SENTINEL_DESKTOP_QML_FILES
    ${CMAKE_SOURCE_DIR}/ui/qml/Main.qml
)

set_source_files_properties(${SENTINEL_DESKTOP_QML_FILES}
    PROPERTIES
        QT_RESOURCE_ALIAS Main.qml
)

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
    WIN32_EXECUTABLE TRUE
)
