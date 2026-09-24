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
