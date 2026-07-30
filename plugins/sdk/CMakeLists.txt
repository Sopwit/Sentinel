add_library(sentinel_plugin_sdk INTERFACE)

target_include_directories(sentinel_plugin_sdk
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/core/include
)

target_link_libraries(sentinel_plugin_sdk
    INTERFACE
        Qt6::Core
)
