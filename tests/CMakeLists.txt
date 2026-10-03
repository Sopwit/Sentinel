function(add_sentinel_core_test target source)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
        message(FATAL_ERROR "Registered test source does not exist: ${source}")
    endif()

    qt_add_executable(${target}
        ${source}
    )

    target_link_libraries(${target}
        PRIVATE
            sentinel_core
            Qt6::Test
    )

    add_test(NAME ${target} COMMAND $<TARGET_FILE:${target}>)
endfunction()

# Opt-in live request probe; never registered with CTest or run by the build.
qt_add_executable(observation_request_probe EXCLUDE_FROM_ALL helpers/observation_request_probe.cpp)
target_link_libraries(observation_request_probe PRIVATE sentinel_core)
qt_add_executable(provider_certification_probe EXCLUDE_FROM_ALL helpers/provider_certification_probe.cpp)
target_link_libraries(provider_certification_probe PRIVATE sentinel_core)
qt_add_executable(audio_certification_probe EXCLUDE_FROM_ALL helpers/audio_certification_probe.cpp)
target_link_libraries(audio_certification_probe PRIVATE sentinel_core)
if(APPLE)
    set_target_properties(audio_certification_probe PROPERTIES MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/helpers/audio_certification_Info.plist.in")
endif()
qt_add_executable(settings_ui_certification_probe EXCLUDE_FROM_ALL helpers/settings_ui_certification_probe.cpp)
if(APPLE)
    set_target_properties(settings_ui_certification_probe PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "dev.sentinel.SettingsCertification"
        MACOSX_BUNDLE_BUNDLE_NAME "Sentinel Settings Certification")
endif()
target_link_libraries(settings_ui_certification_probe PRIVATE sentinel_desktop_viewmodels Qt6::Quick Qt6::Qml)
file(GLOB_RECURSE CERTIFICATION_QML_FILES "${CMAKE_SOURCE_DIR}/ui/qml/*.qml")
foreach(qml_file IN LISTS CERTIFICATION_QML_FILES)
    get_filename_component(qml_name "${qml_file}" NAME)
    set_source_files_properties("${qml_file}" PROPERTIES QT_RESOURCE_ALIAS "${qml_name}")
endforeach()
qt_add_resources(settings_ui_certification_probe "certification_qml"
    PREFIX "/qt/qml/Sentinel/Desktop" FILES ${CERTIFICATION_QML_FILES})
qt_add_resources(settings_ui_certification_probe "certification_qmldir"
    PREFIX "/qt/qml" BASE "${CMAKE_BINARY_DIR}/apps/sentinel-desktop"
    FILES "${CMAKE_BINARY_DIR}/apps/sentinel-desktop/Sentinel/Desktop/qmldir")
file(GLOB CERTIFICATION_ICON_FILES "${CMAKE_SOURCE_DIR}/resources/icons/tabler/*.svg"
    "${CMAKE_SOURCE_DIR}/resources/icons/lucide/*.svg")
qt_add_resources(settings_ui_certification_probe "certification_icons"
    PREFIX "/" BASE "${CMAKE_SOURCE_DIR}/resources" FILES ${CERTIFICATION_ICON_FILES})
target_compile_definitions(settings_ui_certification_probe PRIVATE
    CERTIFICATION_QML_IMPORT="${CMAKE_BINARY_DIR}/apps/sentinel-desktop"
    CERTIFICATION_TRANSLATIONS="${CMAKE_BINARY_DIR}/apps/sentinel-desktop"
    CERTIFICATION_QML_WRAPPER="${CMAKE_CURRENT_SOURCE_DIR}/helpers/settings_ui_certification.qml")

add_sentinel_core_test(test_mode_manager core/test_mode_manager.cpp)
add_sentinel_core_test(test_unified_audio_lifecycle core/test_unified_audio_lifecycle.cpp)
add_sentinel_core_test(test_persistence_startup_certification core/test_persistence_startup_certification.cpp)
add_sentinel_core_test(test_chat_session core/test_chat_session.cpp)
add_sentinel_core_test(test_chat_mode_service core/test_chat_mode_service.cpp)
add_sentinel_core_test(test_chat_application_controller core/test_chat_application_controller.cpp)
add_sentinel_core_test(test_conversation_session core/test_conversation_session.cpp)
add_sentinel_core_test(test_conversation_state_graph core/test_conversation_state_graph.cpp)
add_sentinel_core_test(test_context_assembly core/test_context_assembly.cpp)
add_sentinel_core_test(test_semantic_retrieval core/test_semantic_retrieval.cpp)
add_sentinel_core_test(test_local_rag_store core/test_local_rag_store.cpp)
add_sentinel_core_test(test_in_memory_conversation_store core/test_in_memory_conversation_store.cpp)
add_sentinel_core_test(test_in_memory_memory_candidate_store
                       core/test_in_memory_memory_candidate_store.cpp)
add_sentinel_core_test(test_in_memory_store core/test_in_memory_store.cpp)
add_sentinel_core_test(test_sqlite_chat_history_store core/test_sqlite_chat_history_store.cpp)
add_sentinel_core_test(test_sqlite_conversation_store core/test_sqlite_conversation_store.cpp)
add_sentinel_core_test(test_sqlite_memory_store core/test_sqlite_memory_store.cpp)
add_sentinel_core_test(test_in_memory_tool_registry core/test_in_memory_tool_registry.cpp)
add_sentinel_core_test(test_mcp_tool_provider core/test_mcp_tool_provider.cpp)
qt_add_executable(test_mcp_server helpers/test_mcp_server.cpp)
target_link_libraries(test_mcp_server PRIVATE Qt6::Core)
add_sentinel_core_test(test_mcp_integration core/test_mcp_integration.cpp)
add_dependencies(test_mcp_integration test_mcp_server)
target_compile_definitions(test_mcp_integration PRIVATE
    TEST_MCP_SERVER_PATH="$<TARGET_FILE:test_mcp_server>")
add_sentinel_core_test(test_plugin_tool_integration core/test_plugin_tool_integration.cpp)
qt_add_plugin(broker_certification_plugin SHARED CLASS_NAME BrokerCertificationPlugin
    helpers/broker_certification_plugin.cpp)
target_link_libraries(broker_certification_plugin PRIVATE sentinel_plugin_sdk Qt6::Core)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/broker-fixture")
configure_file(fixtures/plugins/broker/plugin.json "${CMAKE_CURRENT_BINARY_DIR}/broker-fixture/plugin.json" COPYONLY)
add_custom_command(TARGET broker_certification_plugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:broker_certification_plugin>"
        "${CMAKE_CURRENT_BINARY_DIR}/broker-fixture/$<TARGET_FILE_NAME:broker_certification_plugin>")
add_dependencies(test_plugin_tool_integration broker_certification_plugin)
target_compile_definitions(test_plugin_tool_integration PRIVATE
    TEST_BROKER_PLUGIN_DIR="${CMAKE_CURRENT_BINARY_DIR}/broker-fixture")
add_dependencies(test_plugin_tool_integration custom_agent_tool_plugin second_agent_tool_plugin)
add_dependencies(test_plugin_tool_integration stage_sample_agent_plugins)
target_compile_definitions(test_plugin_tool_integration PRIVATE
    TEST_PLUGIN_PATH="$<TARGET_FILE_DIR:custom_agent_tool_plugin>/custom-tool/$<TARGET_FILE_NAME:custom_agent_tool_plugin>")
add_sentinel_core_test(test_agent_activity_log core/test_agent_activity_log.cpp)
add_sentinel_core_test(test_agent_pipeline_result core/test_agent_pipeline_result.cpp)
add_sentinel_core_test(test_agent_runtime_context core/test_agent_runtime_context.cpp)
add_sentinel_core_test(test_agent_task_runtime core/test_agent_task_runtime.cpp)
add_sentinel_core_test(test_controlled_agent_tasks core/test_controlled_agent_tasks.cpp)
add_sentinel_core_test(test_orchestration_snapshot core/test_orchestration_snapshot.cpp)
add_sentinel_core_test(test_static_orchestration_diagnostics
                       core/test_static_orchestration_diagnostics.cpp)
add_sentinel_core_test(test_static_approval_policy core/test_static_approval_policy.cpp)
add_sentinel_core_test(test_static_sandbox_policy core/test_static_sandbox_policy.cpp)
add_sentinel_core_test(test_static_model_router core/test_static_model_router.cpp)
add_sentinel_core_test(test_static_provider_catalog core/test_static_provider_catalog.cpp)
add_sentinel_core_test(test_static_memory_catalog core/test_static_memory_catalog.cpp)
add_sentinel_core_test(test_static_agent_registry core/test_static_agent_registry.cpp)
add_sentinel_core_test(test_static_task_planner core/test_static_task_planner.cpp)
add_sentinel_core_test(test_static_runtime_capability_registry
                       core/test_static_runtime_capability_registry.cpp)
add_sentinel_core_test(test_static_runtime_permission_policy
                       core/test_static_runtime_permission_policy.cpp)
add_sentinel_core_test(test_static_runtime_safety_policy
                       core/test_static_runtime_safety_policy.cpp)
add_sentinel_core_test(test_static_runtime_pipeline core/test_static_runtime_pipeline.cpp)
add_sentinel_core_test(test_static_execution_lifecycle core/test_static_execution_lifecycle.cpp)
add_sentinel_core_test(test_runtime_integration core/test_runtime_integration.cpp)
add_sentinel_core_test(test_runtime_provider core/test_runtime_provider.cpp)
add_sentinel_core_test(test_credential_store core/test_credential_store.cpp)
add_sentinel_core_test(test_provider_credentials core/test_provider_credentials.cpp)
add_sentinel_core_test(test_companion_service core/test_companion_service.cpp)
add_sentinel_core_test(test_permission_policy_service core/test_permission_policy_service.cpp)
add_sentinel_core_test(test_tool_execution_gateway core/test_tool_execution_gateway.cpp)
add_sentinel_core_test(test_tool_argument_validator core/test_tool_argument_validator.cpp)
add_sentinel_core_test(test_agent_runtime_service core/test_agent_runtime_service.cpp)
add_sentinel_core_test(test_workspace_service core/test_workspace_service.cpp)
add_sentinel_core_test(test_worktree_service core/test_worktree_service.cpp)
add_sentinel_core_test(test_skill_profile_service core/test_skill_profile_service.cpp)
add_sentinel_core_test(test_skill_extension_runtime core/test_skill_extension_runtime.cpp)
target_compile_definitions(test_skill_extension_runtime PRIVATE
    TEST_SKILL_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/skills")
add_sentinel_core_test(test_model_registry core/test_model_registry.cpp)
add_sentinel_core_test(test_ollama_runtime core/test_ollama_runtime.cpp)
add_sentinel_core_test(test_local_inference core/test_local_inference.cpp)
add_sentinel_core_test(test_voice core/test_voice.cpp)
add_sentinel_core_test(test_null_tool_executor core/test_null_tool_executor.cpp)
add_sentinel_core_test(test_local_runtime core/test_local_runtime.cpp)
add_sentinel_core_test(test_local_runtime_session core/test_local_runtime_session.cpp)
add_sentinel_core_test(test_standard_path_provider core/test_standard_path_provider.cpp)
add_sentinel_core_test(test_application_controller core/test_application_controller.cpp)
add_sentinel_core_test(test_app_settings core/test_app_settings.cpp)
add_sentinel_core_test(test_settings_recovery_runtime core/test_settings_recovery_runtime.cpp)
target_compile_definitions(test_settings_recovery_runtime PRIVATE
    CERTIFICATION_TRANSLATIONS="${CMAKE_BINARY_DIR}/apps/sentinel-desktop")
add_sentinel_core_test(test_json_settings_store core/test_json_settings_store.cpp)
add_sentinel_core_test(test_keyed_mutex core/test_keyed_mutex.cpp)
add_sentinel_core_test(test_scoped_transform core/test_scoped_transform.cpp)
add_sentinel_core_test(test_token_estimator core/test_token_estimator.cpp)
add_sentinel_core_test(test_fnv_hash core/test_fnv_hash.cpp)
add_sentinel_core_test(test_sampled_checksum core/test_sampled_checksum.cpp)
add_sentinel_core_test(test_lazy core/test_lazy.cpp)
add_sentinel_core_test(test_memo_map core/test_memo_map.cpp)
add_sentinel_core_test(test_flock_file_lock core/test_flock_file_lock.cpp)
add_sentinel_core_test(test_runtime_feature_flags core/test_runtime_feature_flags.cpp)
add_sentinel_core_test(test_json_schema_validator core/test_json_schema_validator.cpp)
add_sentinel_core_test(test_tool_name_validator core/test_tool_name_validator.cpp)
add_sentinel_core_test(test_plugin_hook_registry core/test_plugin_hook_registry.cpp)
add_sentinel_core_test(test_location_service_map core/test_location_service_map.cpp)
add_sentinel_core_test(test_session_execution_patterns core/test_session_execution_patterns.cpp)
add_sentinel_core_test(test_security_utilities core/test_security_utilities.cpp)
add_sentinel_core_test(test_config_resolver core/test_config_resolver.cpp)
add_sentinel_core_test(test_codemode_interpreter core/test_codemode_interpreter.cpp)
add_sentinel_core_test(test_background_job_persistence core/test_background_job_persistence.cpp)
add_sentinel_core_test(test_health_service core/test_health_service.cpp)
add_sentinel_core_test(test_upgrade core/test_upgrade.cpp)
add_sentinel_core_test(test_local_echo_provider core/test_local_echo_provider.cpp)
add_sentinel_core_test(test_null_agent_runtime core/test_null_agent_runtime.cpp)
add_sentinel_core_test(test_agent_loop core/test_agent_loop.cpp)
add_sentinel_core_test(test_agent_runtime core/test_agent_runtime.cpp)
add_sentinel_core_test(test_llm_agent_runtime core/test_llm_agent_runtime.cpp)
add_sentinel_core_test(test_observation_policy core/test_observation_policy.cpp)
add_sentinel_core_test(test_real_tool_executor_tools core/test_real_tool_executor_tools.cpp)
add_sentinel_core_test(test_process_executor core/test_process_executor.cpp)
add_sentinel_core_test(test_fuzzy_editor core/test_fuzzy_editor.cpp)
add_sentinel_core_test(test_web_search_tool core/test_web_search_tool.cpp)
add_sentinel_core_test(test_plugin_manager core/test_plugin_manager.cpp)
add_sentinel_core_test(test_default_platform_services test_default_platform_services.cpp)
add_sentinel_core_test(test_desktop_shell_view_model desktop/test_desktop_shell_view_model.cpp)
add_sentinel_core_test(test_chat_view_model desktop/test_chat_view_model.cpp)
add_sentinel_core_test(test_chat_message_recovery desktop/test_chat_message_recovery.cpp)
target_link_libraries(test_chat_message_recovery PRIVATE sentinel_desktop_viewmodels)

qt_add_executable(test_daemon_ipc
    core/test_daemon_ipc.cpp
)

target_link_libraries(test_daemon_ipc
    PRIVATE
        sentinel_daemon_service
        sentinel_core
        Qt6::Test
)

add_test(NAME test_daemon_ipc COMMAND $<TARGET_FILE:test_daemon_ipc>)

qt_add_executable(test_graphics_backend
    desktop/test_graphics_backend.cpp
)

target_link_libraries(test_graphics_backend
    PRIVATE
        sentinel_desktop_startup
        Qt6::Test
)

add_test(NAME test_graphics_backend COMMAND $<TARGET_FILE:test_graphics_backend>)

target_link_libraries(test_desktop_shell_view_model
    PRIVATE
        sentinel_desktop_viewmodels
)

target_link_libraries(test_chat_view_model
    PRIVATE
        sentinel_desktop_viewmodels
)

# ── Integration, Performance, Benchmark & GUI Test Targets ───────────────────
add_sentinel_core_test(test_rag_e2e_pipeline integration/test_rag_e2e_pipeline.cpp)
add_sentinel_core_test(test_startup_time performance/test_startup_time.cpp)
add_sentinel_core_test(bench_semantic_search benchmarks/bench_semantic_search.cpp)
add_sentinel_core_test(bench_context_compression benchmarks/bench_context_compression.cpp)
add_sentinel_core_test(bench_sqlite_store benchmarks/bench_sqlite_store.cpp)

find_package(Qt6 COMPONENTS QuickTest QUIET)
if(Qt6QuickTest_FOUND AND TARGET Qt6::QuickTest)
    qt_add_executable(test_gui_components
        gui/tst_main.cpp
    )
    target_link_libraries(test_gui_components
        PRIVATE
            Qt6::QuickTest
            Qt6::Quick
            Qt6::Test
    )
    add_test(NAME test_gui_components COMMAND $<TARGET_FILE:test_gui_components> -input "${CMAKE_CURRENT_SOURCE_DIR}/gui")
endif()
