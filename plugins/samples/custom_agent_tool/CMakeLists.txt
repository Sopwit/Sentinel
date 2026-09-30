qt_add_plugin(custom_agent_tool_plugin
    SHARED
    CLASS_NAME CustomToolPlugin
    CustomToolPlugin.h
    CustomToolPlugin.cpp
)

target_link_libraries(custom_agent_tool_plugin
    PRIVATE
        sentinel_plugin_sdk
        Qt6::Core
)

qt_add_plugin(second_agent_tool_plugin
    SHARED
    CLASS_NAME CustomToolPlugin
    CustomToolPlugin.h
    CustomToolPlugin.cpp
)
target_compile_definitions(second_agent_tool_plugin PRIVATE
    SENTINEL_SAMPLE_SECOND_PLUGIN
    SENTINEL_SAMPLE_PLUGIN_MANIFEST="plugin-second.json"
)
target_link_libraries(second_agent_tool_plugin PRIVATE sentinel_plugin_sdk Qt6::Core)

# Stage manifest and binary together: the out-of-process plugin manager discovers
# only explicit plugin directories and never inspects libraries in the desktop.
foreach(sample IN ITEMS custom second)
    if(sample STREQUAL "custom")
        set(sample_target custom_agent_tool_plugin)
        set(sample_manifest plugin.json)
    else()
        set(sample_target second_agent_tool_plugin)
        set(sample_manifest plugin-second.json)
    endif()
    set(sample_dir "${CMAKE_CURRENT_BINARY_DIR}/${sample}-tool")
    file(MAKE_DIRECTORY "${sample_dir}")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/${sample_manifest}"
                   "${sample_dir}/plugin.json" COPYONLY)
    add_custom_command(TARGET ${sample_target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:${sample_target}>" "${sample_dir}/$<TARGET_FILE_NAME:${sample_target}>")
endforeach()
add_custom_target(stage_sample_agent_plugins ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:custom_agent_tool_plugin>"
            "${CMAKE_CURRENT_BINARY_DIR}/custom-tool/$<TARGET_FILE_NAME:custom_agent_tool_plugin>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:second_agent_tool_plugin>"
            "${CMAKE_CURRENT_BINARY_DIR}/second-tool/$<TARGET_FILE_NAME:second_agent_tool_plugin>"
    DEPENDS custom_agent_tool_plugin second_agent_tool_plugin)
