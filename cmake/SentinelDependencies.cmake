# cmake/SentinelDependencies.cmake
# Centralized dependency management infrastructure for Sentinel using FetchContent / CPM

include(FetchContent)

# Set common FetchContent options
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "Disable automatic update checks for FetchContent dependencies")

# Helper to register third-party dependencies if required in the future
function(sentinel_add_dependency name)
    message(STATUS "Sentinel dependency manager initialized for: ${name}")
endfunction()
