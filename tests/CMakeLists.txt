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
add_sentinel_core_test(test_chat_session core/test_chat_session.cpp)
add_sentinel_core_test(test_conversation_session core/test_conversation_session.cpp)
add_sentinel_core_test(test_conversation_state_graph core/test_conversation_state_graph.cpp)
add_sentinel_core_test(test_context_assembly core/test_context_assembly.cpp)
add_sentinel_core_test(test_semantic_retrieval core/test_semantic_retrieval.cpp)
add_sentinel_core_test(test_in_memory_conversation_store core/test_in_memory_conversation_store.cpp)
add_sentinel_core_test(test_in_memory_memory_candidate_store
                       core/test_in_memory_memory_candidate_store.cpp)
add_sentinel_core_test(test_in_memory_store core/test_in_memory_store.cpp)
add_sentinel_core_test(test_sqlite_chat_history_store core/test_sqlite_chat_history_store.cpp)
add_sentinel_core_test(test_sqlite_conversation_store core/test_sqlite_conversation_store.cpp)
add_sentinel_core_test(test_sqlite_memory_store core/test_sqlite_memory_store.cpp)
add_sentinel_core_test(test_in_memory_tool_registry core/test_in_memory_tool_registry.cpp)
add_sentinel_core_test(test_agent_activity_log core/test_agent_activity_log.cpp)
add_sentinel_core_test(test_agent_pipeline_result core/test_agent_pipeline_result.cpp)
add_sentinel_core_test(test_agent_runtime_context core/test_agent_runtime_context.cpp)
add_sentinel_core_test(test_agent_task_runtime core/test_agent_task_runtime.cpp)
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
add_sentinel_core_test(test_json_settings_store core/test_json_settings_store.cpp)
add_sentinel_core_test(test_local_echo_provider core/test_local_echo_provider.cpp)
add_sentinel_core_test(test_null_agent_runtime core/test_null_agent_runtime.cpp)
add_sentinel_core_test(test_desktop_shell_view_model desktop/test_desktop_shell_view_model.cpp)

target_link_libraries(test_desktop_shell_view_model
    PRIVATE
        sentinel_desktop_viewmodels
)
