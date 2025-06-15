cmake_minimum_required(VERSION 3.16)
project(SolderLib)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Threads REQUIRED)

include(FetchContent)

# Fetch PhotonLibOS
set(PHOTON_ENABLE_URING OFF CACHE INTERNAL "Enable iouring")
set(PHOTON_CXX_STANDARD 14 CACHE INTERNAL "C++ standard")

FetchContent_Declare(
    photon
    GIT_REPOSITORY https://github.com/fuji-184/PhotonLibOS.git
    GIT_TAG main
)
FetchContent_MakeAvailable(photon)

# Fetch mimalloc
FetchContent_Declare(
    mimalloc
    GIT_REPOSITORY https://github.com/microsoft/mimalloc.git
    GIT_TAG v2.1.2
    GIT_SHALLOW TRUE
)

set(MI_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(MI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(mimalloc)

# Create the main library
add_library(solder_lib STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/solder/picohttpparser.c
    ${CMAKE_CURRENT_SOURCE_DIR}/include/solder/http_types.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/solder/parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/solder/router.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/solder/server.cpp
)

target_include_directories(solder_lib PUBLIC
    $<BUILD_INTERFACE:${photon_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(solder_lib PRIVATE
    photon_static
    mimalloc-static
    Threads::Threads
)

target_compile_options(solder_lib PRIVATE
    -O3 -march=native -mtune=native -flto -ffast-math
    -fstrict-aliasing -fomit-frame-pointer -DNDEBUG
    -Wall -Wextra -Wno-unused-parameter
)

target_link_options(solder_lib PRIVATE -flto)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(solder_lib PRIVATE -fcoroutines)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(solder_lib PRIVATE -stdlib=libc++)
endif()

# Create output directories
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/dist/lib)
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/dist/include)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/temp)

# Set library output name and location
set_target_properties(solder_lib PROPERTIES
    OUTPUT_NAME solder
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/dist/lib
)

# Custom target to copy headers after build
add_custom_command(TARGET solder_lib POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Copying headers..."
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/dist/include
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${photon_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/dist/include/photon
    COMMENT "Copying headers to dist directory"
)

# Create combined library with all dependencies
add_custom_command(TARGET solder_lib POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Creating combined library..."
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/temp
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/temp
    COMMAND cd ${CMAKE_BINARY_DIR}/temp && ${CMAKE_AR} x $<TARGET_FILE:photon_static>
    COMMAND cd ${CMAKE_BINARY_DIR}/temp && ${CMAKE_AR} x $<TARGET_FILE:mimalloc-static>
    COMMAND cd ${CMAKE_BINARY_DIR}/temp && ${CMAKE_AR} x $<TARGET_FILE:solder_lib>
    COMMAND cd ${CMAKE_BINARY_DIR}/temp && ${CMAKE_AR} rcs ${CMAKE_SOURCE_DIR}/dist/lib/libsolder_combined.a *.o
    COMMENT "Creating combined static library"
)

# Create a simple usage info file using file(WRITE) instead of echo commands
add_custom_command(TARGET solder_lib POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Creating usage info..."
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/create_build_info.cmake
    COMMENT "Creating build info"
)

# Generate the script to create BUILD_INFO.txt
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/create_build_info.cmake "
file(WRITE \"${CMAKE_SOURCE_DIR}/dist/BUILD_INFO.txt\" \"SolderLib Build Complete

Library files:
  - libsolder.a (regular library)
  - libsolder_combined.a (with all dependencies)

Headers are in: dist/include/
PhotonLibOS headers are in: dist/include/photon/
\")
")
