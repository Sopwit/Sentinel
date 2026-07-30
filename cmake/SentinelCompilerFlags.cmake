# cmake/SentinelCompilerFlags.cmake
# Compiler flags, strict diagnostics, C++20 standard enforcement and LTO for Sentinel

option(SENTINEL_ENABLE_WERROR "Treat compiler warnings as errors" OFF)
option(SENTINEL_ENABLE_LTO "Enable Link-Time Optimization (IPO/LTO) for Release builds" OFF)

function(sentinel_apply_compiler_flags target_name)
    if(NOT TARGET ${target_name})
        return()
    endif()

    target_compile_features(${target_name} PUBLIC cxx_std_20)

    if(MSVC)
        target_compile_options(${target_name} PRIVATE
            /W4
            /permissive-
            /utf-8
            $<$<BOOL:${SENTINEL_ENABLE_WERROR}>:/WX>
        )
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        target_compile_options(${target_name} PRIVATE
            -Wall
            -Wextra
            -Wnon-virtual-dtor
            -Wshadow
            -Wcast-align
            -Wunused
            -Woverloaded-virtual
            -Wno-missing-field-initializers
            -Wno-unused-parameter
            -Wno-unused-lambda-capture
            $<$<BOOL:${SENTINEL_ENABLE_WERROR}>:-Werror>
        )
    endif()

    if(SENTINEL_ENABLE_LTO)
        set_target_properties(${target_name} PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endfunction()
