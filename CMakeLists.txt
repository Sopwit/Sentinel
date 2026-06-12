cmake_minimum_required(VERSION 3.24)

project(Sentinel
    VERSION 0.1.0
    DESCRIPTION "Sentinel Desktop Alpha foundation"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Quick Qml Sql Test Network Widgets)

qt_standard_project_setup(REQUIRES 6.5)

add_subdirectory(core)
add_subdirectory(apps/sentinel-desktop)

enable_testing()
add_subdirectory(tests)
