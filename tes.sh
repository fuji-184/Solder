#!/bin/bash
set -e

echo "=== SolderLib Complete Build Script ==="
echo

# Get current directory
PROJECT_ROOT=$(pwd)

# Step 1: Check dependencies
echo "Step 1: Checking system dependencies..."
echo "Checking for required packages..."

# Check for OpenSSL development headers
if ! pkg-config --exists openssl; then
    echo "ERROR: OpenSSL development headers not found!"
    echo "Please install: sudo apt-get install libssl-dev (Ubuntu/Debian) or equivalent"
    exit 1
fi

# Check for other common dependencies
MISSING_DEPS=""
if ! pkg-config --exists zlib; then
    MISSING_DEPS="$MISSING_DEPS zlib1g-dev"
fi

if [ ! -z "$MISSING_DEPS" ]; then
    echo "WARNING: Some optional dependencies might be missing: $MISSING_DEPS"
    echo "Consider installing with: sudo apt-get install $MISSING_DEPS"
fi

echo "✓ Dependencies check completed"
echo

# Step 2: Build library
echo "Step 2: Building SolderLib..."
echo "Current directory: $PROJECT_ROOT"

# Clean previous builds
rm -rf build_lib dist

# Create build directory for library
mkdir -p build_lib
cd build_lib

echo "Configuring library build..."
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_CXX_STANDARD=20 \
         -DCMAKE_POSITION_INDEPENDENT_CODE=ON

echo "Building library (this may take a while)..."
make -j$(nproc)

cd "$PROJECT_ROOT"

# Check if build was successful
if [ ! -f "dist/lib/libsolder.a" ]; then
    echo "ERROR: Library build failed - libsolder.a not found"
    exit 1
fi

echo "✓ Library build successful!"
echo

# Step 3: Show what was built
echo "=== Build Results ==="
echo "Libraries created:"
ls -lh dist/lib/
echo
echo "Headers available:"
ls -la dist/include/
echo
if [ -d "dist/include/photon" ]; then
    echo "PhotonLibOS headers:"
    ls -la dist/include/photon/ | head -10
    echo "... (and more)"
else
    echo "Note: PhotonLibOS headers not found in expected location"
fi
echo

# Step 4: Create improved CMake config
echo "Step 3: Creating improved CMake configuration..."

# Create improved SolderLibConfig.cmake
cat > dist/SolderLibConfig.cmake << 'EOF'
# SolderLibConfig.cmake
# Configuration file for SolderLib

# Get the directory where this config file is located
get_filename_component(SolderLib_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)

# Define paths relative to config file location
set(SolderLib_LIB_DIR "${SolderLib_CMAKE_DIR}/lib")
set(SolderLib_INCLUDE_DIR "${SolderLib_CMAKE_DIR}/include")

# Find required system dependencies
find_package(PkgConfig REQUIRED)
pkg_check_modules(OPENSSL REQUIRED openssl)

find_package(Threads REQUIRED)

# Check if zlib is available
pkg_check_modules(ZLIB zlib)

# Create the imported target
if(NOT TARGET SolderLib::solder)
    add_library(SolderLib::solder STATIC IMPORTED)

    # Determine which library file to use
    if(EXISTS "${SolderLib_LIB_DIR}/libsolder_combined.a")
        set(SOLDER_LIB_FILE "${SolderLib_LIB_DIR}/libsolder_combined.a")
        message(STATUS "Using combined SolderLib: ${SOLDER_LIB_FILE}")
    elseif(EXISTS "${SolderLib_LIB_DIR}/libsolder.a")
        set(SOLDER_LIB_FILE "${SolderLib_LIB_DIR}/libsolder.a")
        message(STATUS "Using regular SolderLib: ${SOLDER_LIB_FILE}")
    else()
        message(FATAL_ERROR "SolderLib not found in ${SolderLib_LIB_DIR}")
    endif()

    # Set library location and include directories
    set_target_properties(SolderLib::solder PROPERTIES
        IMPORTED_LOCATION "${SOLDER_LIB_FILE}"
        INTERFACE_INCLUDE_DIRECTORIES "${SolderLib_INCLUDE_DIR};${SolderLib_INCLUDE_DIR}/photon"
    )

    # Link required system dependencies
    target_link_libraries(SolderLib::solder INTERFACE
        Threads::Threads
        ${OPENSSL_LIBRARIES}
    )

    # Add OpenSSL include directories
    target_include_directories(SolderLib::solder INTERFACE ${OPENSSL_INCLUDE_DIRS})
    target_compile_options(SolderLib::solder INTERFACE ${OPENSSL_CFLAGS_OTHER})

    # Add platform-specific libraries
    if(UNIX AND NOT APPLE)
        target_link_libraries(SolderLib::solder INTERFACE rt dl m)
    elseif(UNIX)
        target_link_libraries(SolderLib::solder INTERFACE m)
    endif()

    # Add zlib if available
    if(ZLIB_FOUND)
        target_link_libraries(SolderLib::solder INTERFACE ${ZLIB_LIBRARIES})
        target_include_directories(SolderLib::solder INTERFACE ${ZLIB_INCLUDE_DIRS})
    endif()
endif()

set(SolderLib_FOUND TRUE)

# Provide some useful variables
set(SolderLib_LIBRARIES SolderLib::solder)
set(SolderLib_INCLUDE_DIRS "${SolderLib_INCLUDE_DIR};${SolderLib_INCLUDE_DIR}/photon")

message(STATUS "Found SolderLib: ${SolderLib_LIB_DIR}")
EOF

echo "✓ Improved CMake configuration created"
echo

# Step 5: Create usage example
echo "Step 4: Creating usage documentation..."

cat > dist/README.md << 'EOF'
# SolderLib Usage Guide

## Installation

Copy the entire `dist/` directory to your project or system.

## CMake Integration

### Method 1: Using find_package (Recommended)

```cmake
cmake_minimum_required(VERSION 3.16)
project(my_app CXX)
set(CMAKE_CXX_STANDARD 20)

# Add the path where SolderLibConfig.cmake is located
list(APPEND CMAKE_PREFIX_PATH "/path/to/solderlib/dist")

find_package(SolderLib REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE SolderLib::solder)
```

### Method 2: Direct Integration

```cmake
cmake_minimum_required(VERSION 3.16)
project(my_app CXX)
set(CMAKE_CXX_STANDARD 20)

# Set path to SolderLib
set(SOLDER_ROOT "/path/to/solderlib/dist")

# Include the config
include("${SOLDER_ROOT}/SolderLibConfig.cmake")

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE SolderLib::solder)
```

## System Requirements

- OpenSSL development headers (`libssl-dev` on Ubuntu/Debian)
- CMake 3.16 or newer
- C++20 compatible compiler
- pkg-config

## Basic Usage Example

```cpp
#include <solder/solder.hpp>
#include <photon/photon.h>
#include <iostream>

int main() {
    // Initialize PhotonLibOS
    if (photon::init(photon::INIT_EVENT_DEFAULT, photon::INIT_IO_DEFAULT) < 0) {
        std::cerr << "Failed to initialize PhotonLibOS" << std::endl;
        return -1;
    }

    std::cout << "SolderLib initialized successfully!" << std::endl;

    // Your code here...

    // Cleanup
    photon::fini();
    return 0;
}
```

## Troubleshooting

1. **SSL/TLS linking errors**: Make sure OpenSSL development headers are installed
2. **Missing headers**: Ensure the include paths point to both `include/` and `include/photon/`
3. **Linking errors**: Use the provided CMake configuration which handles all dependencies

## Files Included

- `lib/libsolder.a` - Main SolderLib static library
- `lib/libsolder_combined.a` - Combined library (if available)
- `include/` - All header files including PhotonLibOS headers
- `SolderLibConfig.cmake` - CMake configuration file
EOF

echo "✓ Usage documentation created"
echo

echo "=== BUILD COMPLETE ==="
echo
echo "What was created:"
echo "📁 dist/lib/ - Static libraries"
echo "📁 dist/include/ - All headers including PhotonLibOS"
echo "📁 dist/SolderLibConfig.cmake - CMake configuration with dependency handling"
echo "📁 dist/README.md - Usage guide and documentation"
echo
echo "To use in your projects:"
echo "1. Copy the dist/ directory to your project or install location"
echo "2. Use find_package(SolderLib REQUIRED) in your CMakeLists.txt"
echo "3. Link with target_link_libraries(your_target PRIVATE SolderLib::solder)"
echo
echo "🎉 Success! SolderLib is ready to use!"
echo
echo "Note: Make sure you have OpenSSL development headers installed:"
echo "  Ubuntu/Debian: sudo apt-get install libssl-dev"
echo "  CentOS/RHEL: sudo yum install openssl-devel"
echo "  macOS: brew install openssl"
