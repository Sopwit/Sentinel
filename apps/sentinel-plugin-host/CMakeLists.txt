qt_add_executable(sentinel-plugin-host
    main.cpp
    ${CMAKE_SOURCE_DIR}/core/src/plugin/PluginManifest.cpp
    ${CMAKE_SOURCE_DIR}/core/src/plugin/PluginPermissions.cpp
)
target_link_libraries(sentinel-plugin-host PRIVATE sentinel_plugin_sdk Qt6::Core)
target_include_directories(sentinel-plugin-host PRIVATE ${CMAKE_SOURCE_DIR}/core/include)
target_compile_features(sentinel-plugin-host PRIVATE cxx_std_20)
