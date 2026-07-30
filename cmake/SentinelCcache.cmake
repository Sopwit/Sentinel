# cmake/SentinelCcache.cmake
# Auto-detect ccache or sccache for build acceleration

option(SENTINEL_USE_CCACHE "Enable ccache/sccache build compiler launcher" ON)

if(SENTINEL_USE_CCACHE)
    find_program(CCACHE_PROGRAM ccache)
    find_program(SCCACHE_PROGRAM sccache)

    if(CCACHE_PROGRAM)
        message(STATUS "Found ccache: ${CCACHE_PROGRAM}")
        set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}" CACHE STRING "C compiler launcher" FORCE)
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}" CACHE STRING "CXX compiler launcher" FORCE)
    elseif(SCCACHE_PROGRAM)
        message(STATUS "Found sccache: ${SCCACHE_PROGRAM}")
        set(CMAKE_C_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}" CACHE STRING "C compiler launcher" FORCE)
        set(CMAKE_CXX_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}" CACHE STRING "CXX compiler launcher" FORCE)
    else()
        message(STATUS "Neither ccache nor sccache found. Proceeding without compiler cache.")
    endif()
endif()
