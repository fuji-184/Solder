# cmake_policy(VERSION 3.5)
cmake_minimum_required(VERSION 3.16)
project(ObengLib)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Threads REQUIRED)

include(FetchContent)
include(ExternalProject)

# Try to find system libpq first
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(LIBPQ QUIET libpq)
endif()

if(LIBPQ_FOUND)
    # Use system libpq
    add_library(libpq INTERFACE)
    target_include_directories(libpq INTERFACE ${LIBPQ_INCLUDE_DIRS})
    target_link_libraries(libpq INTERFACE ${LIBPQ_LIBRARIES})
    message(STATUS "Using system libpq: ${LIBPQ_VERSION}")
else()
    # Build PostgreSQL from source with proper configuration
    ExternalProject_Add(libpq_external
      PREFIX ${CMAKE_BINARY_DIR}/libpq
      GIT_REPOSITORY https://github.com/postgres/postgres.git
      GIT_TAG REL_16_1
      CONFIGURE_COMMAND bash -c "CC='gcc -std=c11' CFLAGS='-std=c11' ./configure --prefix=${CMAKE_BINARY_DIR}/libpq/install --without-readline --without-zlib --without-icu --disable-rpath"
      BUILD_COMMAND bash -c "CC='gcc -std=c11' CFLAGS='-std=c11' make -C src/interfaces/libpq all && make -C src/bin/pg_config all && make -C src/include all"
      INSTALL_COMMAND bash -c "make -C src/interfaces/libpq install && make -C src/bin/pg_config install && make -C src/include install"
      BUILD_IN_SOURCE 1
    )

    # Set variables for libpq include and lib paths
    set(LIBPQ_INCLUDE_DIR ${CMAKE_BINARY_DIR}/libpq/install/include)
    set(LIBPQ_LIB_DIR ${CMAKE_BINARY_DIR}/libpq/install/lib)

    # Create directories to avoid CMake errors
    file(MAKE_DIRECTORY ${LIBPQ_INCLUDE_DIR})
    file(MAKE_DIRECTORY ${LIBPQ_LIB_DIR})

    # Create a simple interface library instead of imported target
    add_library(libpq INTERFACE)
    add_dependencies(libpq libpq_external)

    # Set the include directories and link libraries
    target_include_directories(libpq INTERFACE ${LIBPQ_INCLUDE_DIR})
    target_link_libraries(libpq INTERFACE ${LIBPQ_LIB_DIR}/libpq.a)
endif()


set(PHOTON_ENABLE_URING OFF CACHE INTERNAL "Enable iouring")
set(PHOTON_CXX_STANDARD 14 CACHE INTERNAL "C++ standard")

FetchContent_Declare(
    photon
    GIT_REPOSITORY https://github.com/alibaba/PhotonLibOS.git
    GIT_TAG v0.9.0
)
FetchContent_MakeAvailable(photon)


# Fetch mimalloc
FetchContent_Declare(
    mimalloc
    GIT_REPOSITORY https://github.com/microsoft/mimalloc.git
    GIT_TAG v2.1.2 # pilih versi stabil terbaru
    GIT_SHALLOW TRUE
)

set(MI_BUILD_SHARED OFF CACHE BOOL "" FORCE)  # Hanya build static lib
set(MI_BUILD_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(mimalloc)

add_library(obeng_lib STATIC
    # C parser file
    ${CMAKE_CURRENT_SOURCE_DIR}/include/obeng/picohttpparser.c
    # ALL C++ implementation files
    ${CMAKE_CURRENT_SOURCE_DIR}/include/obeng/http_types.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/obeng/parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/obeng/router.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/obeng/server.cpp
)

target_include_directories(obeng_lib PUBLIC
    ${photonlib_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link with libcoro and other dependencies
target_link_libraries(obeng_lib PUBLIC
    photon_static
    Threads::Threads
    mimalloc-static
    libpq
)

target_compile_options(obeng_lib PRIVATE
    -O3                                # Optimisasi level tertinggi
    -march=native                      # Gunakan semua instruksi CPU lokal
    -mtune=native                      # Tuning untuk performa CPU lokal
    -flto                              # Link Time Optimization (cross TU optimisasi)
    -ffast-math                        # Percepat operasi math, non-standar IEEE
    -fstrict-aliasing                  # Aktifkan asumsi strict aliasing
    -fomit-frame-pointer               # Hilangkan frame pointer (lebih banyak register)
    -DNDEBUG                           # Disable assert & debug info
    -Wall -Wextra -Wno-unused-parameter # Warning set (tetap aman)
)

target_link_options(obeng_lib PRIVATE
    -flto
)

# Enable coroutines for C++ files
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(obeng_lib PRIVATE -fcoroutines)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(obeng_lib PRIVATE -stdlib=libc++)
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(obeng_lib PRIVATE rt)
endif()

set_target_properties(obeng_lib PROPERTIES
    OUTPUT_NAME obeng
    PREFIX ""
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/obeng/include
)

# Install rules
install(TARGETS obeng_lib
    ARCHIVE DESTINATION lib
)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION include)
