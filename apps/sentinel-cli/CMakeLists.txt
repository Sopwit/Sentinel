add_executable(sentinel-cli
    main.cpp
    commands/ChatCommand.h
    commands/ChatCommand.cpp
    commands/ModelCommand.h
    commands/ModelCommand.cpp
    commands/StatusCommand.h
    commands/StatusCommand.cpp
    commands/ConfigCommand.h
    commands/ConfigCommand.cpp
)

target_link_libraries(sentinel-cli
    PRIVATE
        sentinel_core
        Qt6::Core
        Qt6::Network
)

sentinel_apply_compiler_flags(sentinel-cli)
sentinel_enable_pch(sentinel-cli)

include(GNUInstallDirs)

set(SENTINEL_CLI_INSTALL_DIR "${CMAKE_INSTALL_BINDIR}")
if(WIN32)
    set(SENTINEL_CLI_INSTALL_DIR ".")
endif()

install(TARGETS sentinel-cli
    RUNTIME DESTINATION ${SENTINEL_CLI_INSTALL_DIR}
)
