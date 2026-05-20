cmake_minimum_required(VERSION 3.28)

# Set compiler BEFORE project()

project(mtable LANGUAGES CXX)


set(CLANG21_ROOT /opt/clang-p2996/)

set(CMAKE_BUILD_RPATH
        "${CLANG21_ROOT}/lib"
        "${CLANG21_ROOT}/lib/x86_64-unknown-linux-gnu"
)

set(CMAKE_INSTALL_RPATH
        "${CLANG21_ROOT}/lib"
        "${CLANG21_ROOT}/lib/x86_64-unknown-linux-gnu"
)
# Default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Build flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# Disable modules scanning
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# C++26
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Executable
add_executable(mtable main.cpp)

# libc++ includes
target_link_directories(mtable PRIVATE
        ${CLANG21_ROOT}/lib
        ${CLANG21_ROOT}/lib/x86_64-unknown-linux-gnu
)

target_link_options(mtable PRIVATE
        -stdlib=libc++
        "-Wl,-rpath,${CLANG21_ROOT}/lib"
        "-Wl,-rpath,${CLANG21_ROOT}/lib/x86_64-unknown-linux-gnu"
)
# Library search paths
target_link_directories(mtable PRIVATE
        ${CLANG21_ROOT}1/lib
        ${CLANG21_ROOT}/lib/x86_64-unknown-linux-gnu
)

# Compile flags
target_compile_options(mtable PRIVATE
        -std=c++2c
        -stdlib=libc++
        -freflection
        -fexpansion-statements
        -Wall
        -Wextra
        -Wpedantic
)

# Link flags
target_link_options(mtable PRIVATE
        -stdlib=libc++
)

# libc++ runtime
target_link_libraries(mtable PRIVATE
        c++
        c++abi
        unwind
)