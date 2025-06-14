
cmake_minimum_required(VERSION 3.16)
project(server_app CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MATIKAN INTERPROCEDURAL OPTIMIZATION SECARA GLOBAL
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)

find_package(Threads REQUIRED)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

message(STATUS "Fetching libs from GitHub...")
include(FetchContent)

# Jangan aktifkan iouring (sesuai kamu)
set(PHOTON_ENABLE_URING OFF CACHE INTERNAL "Enable iouring")
set(PHOTON_CXX_STANDARD 14 CACHE INTERNAL "C++ standard")

FetchContent_Declare(
    photon
    GIT_REPOSITORY https://github.com/alibaba/PhotonLibOS.git
    GIT_TAG v0.9.0
)
FetchContent_MakeAvailable(photon)

FetchContent_Declare(
    mimalloc
    GIT_REPOSITORY https://github.com/microsoft/mimalloc.git
    GIT_TAG v2.1.2
    GIT_SHALLOW TRUE
)
set(MI_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(MI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(mimalloc)

set(OBENG "${CMAKE_CURRENT_SOURCE_DIR}/include/obeng.a")

if(NOT EXISTS ${OBENG})
    message(FATAL_ERROR "Obeng not found at: ${OBENG}")
endif()

message(STATUS "Using Obeng: ${OBENG}")

add_executable(server_app main.cpp)

target_include_directories(server_app PRIVATE
    ${photon_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/obeng
)

target_link_libraries(server_app PRIVATE
    ${OBENG}
    photon_static
    Threads::Threads
    mimalloc-static
    pq
)

# **MATIKAN LTO UNTUK TARGET INI SECARA EXPLISIT**
set_target_properties(server_app PROPERTIES INTERPROCEDURAL_OPTIMIZATION OFF)

# Link whole archive obeng.a (kamu sudah benar)
target_link_options(server_app PRIVATE
    -Wl,--whole-archive,${OBENG},--no-whole-archive
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(server_app PRIVATE
        -Wall -Wextra
        -O3
        -march=native
        -mtune=native
        #-flto          # **JANGAN AKTIFKAN LTO KALO MASIH ERROR**
        -ffast-math
        -fcoroutines
        -fstrict-aliasing
        -fomit-frame-pointer
        -DNDEBUG
    )
    # Jangan aktifkan link-time optimization dulu
    # target_link_options(server_app PRIVATE -flto)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(server_app PRIVATE
        -Wall -Wextra
        -O3
        -march=native
        -mtune=native
        #-flto          # Jangan aktifkan dulu
        -ffast-math
        -stdlib=libc++
        -fstrict-aliasing
        -fomit-frame-pointer
        -DNDEBUG
    )
    # target_link_options(server_app PRIVATE -flto)
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(server_app PRIVATE rt)
endif()
