add_library(sentinel_daemon_service STATIC
    service/DaemonIpcServer.h
    service/DaemonIpcServer.cpp
    service/DaemonService.h
    service/DaemonService.cpp
)

target_link_libraries(sentinel_daemon_service
    PUBLIC
        sentinel_core
        Qt6::Core
        Qt6::Network
)

target_include_directories(sentinel_daemon_service
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(sentinel-daemon
    main.cpp
)

target_link_libraries(sentinel-daemon
    PRIVATE
        sentinel_daemon_service
        sentinel_core
        Qt6::Core
        Qt6::Network
)

sentinel_apply_compiler_flags(sentinel_daemon_service)
sentinel_enable_pch(sentinel_daemon_service)
sentinel_apply_compiler_flags(sentinel-daemon)
sentinel_enable_pch(sentinel-daemon)

include(GNUInstallDirs)

set(SENTINEL_DAEMON_INSTALL_DIR "${CMAKE_INSTALL_BINDIR}")
if(WIN32)
    set(SENTINEL_DAEMON_INSTALL_DIR ".")
endif()

install(TARGETS sentinel-daemon
    RUNTIME DESTINATION ${SENTINEL_DAEMON_INSTALL_DIR}
)
