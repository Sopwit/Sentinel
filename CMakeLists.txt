cmake_minimum_required(VERSION 3.24)

project(Sentinel
    VERSION 1.0.0
    DESCRIPTION "Sentinel local-first AI desktop assistant"
    LANGUAGES CXX
)

set(SENTINEL_APP_ID "dev.sentinel.Sentinel" CACHE STRING "Reverse-DNS application id")
set(SENTINEL_DISPLAY_NAME "Sentinel Desktop" CACHE STRING "User-visible application name")
set(SENTINEL_ORGANIZATION_NAME "Sentinel" CACHE STRING "Application organization name")
set(SENTINEL_APP_VERSION "1.0.0-rc.1" CACHE STRING "User-visible application version")
set(SENTINEL_BUILD_NUMBER "0" CACHE STRING "Release build number")
set(SENTINEL_COPYRIGHT "Copyright 2026 Sentinel contributors" CACHE STRING "Application copyright")
set(SENTINEL_MACOS_CATEGORY "public.app-category.productivity" CACHE STRING "macOS App Store category")
set(SENTINEL_MACOS_MIN_VERSION "12.0" CACHE STRING "Minimum supported macOS version for release bundles")
set(SENTINEL_CONFIGURED_BUILD_TYPE "${CMAKE_BUILD_TYPE}")
if(NOT SENTINEL_CONFIGURED_BUILD_TYPE)
    set(SENTINEL_CONFIGURED_BUILD_TYPE "unknown")
endif()

find_package(Git QUIET)
set(SENTINEL_GIT_COMMIT "unknown")
if(Git_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short=12 HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE SENTINEL_GIT_COMMIT_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(SENTINEL_GIT_COMMIT_OUTPUT)
        set(SENTINEL_GIT_COMMIT "${SENTINEL_GIT_COMMIT_OUTPUT}")
    endif()
endif()

if(APPLE AND NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "${SENTINEL_MACOS_MIN_VERSION}" CACHE STRING "Minimum macOS deployment target")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Quick Qml Sql Test Network Widgets LinguistTools)

qt_standard_project_setup(REQUIRES 6.5)

add_subdirectory(core)
add_subdirectory(apps/sentinel-desktop)

enable_testing()
add_subdirectory(tests)
