function(add_sentinel_core_test target source)
    qt_add_executable(${target}
        ${source}
    )

    target_link_libraries(${target}
        PRIVATE
            sentinel_core
            Qt6::Test
    )

    add_test(NAME ${target} COMMAND ${target})
endfunction()

add_sentinel_core_test(test_mode_manager core/test_mode_manager.cpp)
add_sentinel_core_test(test_in_memory_store core/test_in_memory_store.cpp)
add_sentinel_core_test(test_application_controller core/test_application_controller.cpp)
add_sentinel_core_test(test_app_settings core/test_app_settings.cpp)
add_sentinel_core_test(test_json_settings_store core/test_json_settings_store.cpp)
add_sentinel_core_test(test_local_echo_provider core/test_local_echo_provider.cpp)
add_sentinel_core_test(test_desktop_shell_view_model desktop/test_desktop_shell_view_model.cpp)

target_link_libraries(test_desktop_shell_view_model
    PRIVATE
        sentinel_desktop_viewmodels
)
