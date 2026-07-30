qt_add_plugin(ollama_extended_plugin
    SHARED
    CLASS_NAME CustomLlmPlugin
    CustomLlmPlugin.h
    CustomLlmPlugin.cpp
)

target_link_libraries(ollama_extended_plugin
    PRIVATE
        sentinel_plugin_sdk
        Qt6::Core
)
