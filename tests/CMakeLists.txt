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
