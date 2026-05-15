add_library(sentinel_core STATIC
    include/sentinel/core/AppMetadata.h
    include/sentinel/core/AppSettings.h
    include/sentinel/core/ApplicationController.h
    include/sentinel/core/FakeProvider.h
    include/sentinel/core/IContextEngine.h
    include/sentinel/core/IIntegration.h
    include/sentinel/core/IMemoryStore.h
    include/sentinel/core/IPlugin.h
    include/sentinel/core/IProvider.h
    include/sentinel/core/ISettingsStore.h
    include/sentinel/core/InMemoryStore.h
    include/sentinel/core/InMemorySettingsStore.h
    include/sentinel/core/JsonSettingsStore.h
    include/sentinel/core/ModeManager.h
    src/AppMetadata.cpp
    src/AppSettings.cpp
    src/ApplicationController.cpp
    src/FakeProvider.cpp
    src/IContextEngine.cpp
    src/InMemoryStore.cpp
    src/InMemorySettingsStore.cpp
    src/JsonSettingsStore.cpp
    src/ModeManager.cpp
)

target_include_directories(sentinel_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(sentinel_core
    PUBLIC
        Qt6::Core
)

target_compile_features(sentinel_core PUBLIC cxx_std_20)
