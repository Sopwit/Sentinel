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
