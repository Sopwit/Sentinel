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

set(sample_dir "${CMAKE_CURRENT_BINARY_DIR}/ollama-extended")
file(MAKE_DIRECTORY "${sample_dir}")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/plugin.json"
               "${sample_dir}/plugin.json" COPYONLY)
add_custom_command(TARGET ollama_extended_plugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:ollama_extended_plugin>"
            "${sample_dir}/$<TARGET_FILE_NAME:ollama_extended_plugin>")
add_custom_target(stage_sample_llm_plugin ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:ollama_extended_plugin>"
            "${sample_dir}/$<TARGET_FILE_NAME:ollama_extended_plugin>"
    DEPENDS ollama_extended_plugin)
