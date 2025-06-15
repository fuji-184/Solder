cmake_minimum_required(VERSION 3.16)
project(server_app CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(SOLDER_COMBINED "${CMAKE_CURRENT_SOURCE_DIR}/include/solder.a")
if(EXISTS ${SOLDER_COMBINED})
    set(SOLDER_LIB ${SOLDER_COMBINED})
    message(STATUS "Using Solder library: ${SOLDER_LIB}")
else()
    message(FATAL_ERROR "Solder library not found. Tried:\n  ${SOLDER_COMBINED}")
endif()

add_library(SolderLib::solder STATIC IMPORTED)
set_target_properties(SolderLib::solder PROPERTIES
    IMPORTED_LOCATION ${SOLDER_LIB}
)

set(INCLUDE_DIRS "")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include")
    list(APPEND INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include")
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include/solder")
    list(APPEND INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include/solder")
endif()

if(INCLUDE_DIRS)
    target_include_directories(SolderLib::solder INTERFACE ${INCLUDE_DIRS})
    message(STATUS "Using include directories: ${INCLUDE_DIRS}")
else()
    message(WARNING "No include directories found")
endif()

find_package(Threads REQUIRED)
target_link_libraries(SolderLib::solder INTERFACE Threads::Threads)

if(UNIX AND NOT APPLE)
    target_link_libraries(SolderLib::solder INTERFACE rt dl m)
elseif(UNIX)
    target_link_libraries(SolderLib::solder INTERFACE m)
endif()

add_executable(server_app main.cpp)

target_link_libraries(server_app PRIVATE SolderLib::solder)

find_package(CURL REQUIRED)
find_library(LIBAIO_LIBRARY aio)
find_library(SSL_LIBRARY ssl)
find_library(CRYPTO_LIBRARY crypto)

target_link_libraries(server_app PRIVATE
    ${LIBAIO_LIBRARY}
    CURL::libcurl
    ${SSL_LIBRARY}
    ${CRYPTO_LIBRARY}
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(server_app PRIVATE
        -Wall -Wextra
        -O3
        -march=native
        -mtune=native
        -ffast-math
        -fcoroutines
        -fstrict-aliasing
        -fomit-frame-pointer
        -DNDEBUG
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(server_app PRIVATE
        -Wall -Wextra
        -O3
        -march=native
        -mtune=native
        -ffast-math
        -stdlib=libc++
        -fstrict-aliasing
        -fomit-frame-pointer
        -DNDEBUG
    )
endif()
