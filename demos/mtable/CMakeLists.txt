cmake_minimum_required(VERSION 3.28)

project(mtable LANGUAGES CXX)

# Set the C++ compiler to use the clang26 build
set(CMAKE_CXX_COMPILER /home/andrei/Desktop/clang26/build/bin/clang++)

# Set build type to Debug if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Ensure debug symbols are always included
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
# ---- Require C++2c (C++26 draft) ----
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- Create executable ----
add_executable(mtable main.cpp)


target_include_directories(mtable PRIVATE
        /home/andrei/Desktop/clang26/build/include/c++/v1
        /home/andrei/Desktop/clang26/build/include/x86_64-unknown-linux-gnu/c++/v1
)

target_link_directories(mtable PRIVATE
        /home/andrei/Desktop/clang26/build/lib/x86_64-unknown-linux-gnu
)

# link libc++ runtime
target_link_libraries(mtable PRIVATE
        c++
        c++abi
        unwind
)


# ---- Extra warnings (recommended) ----
target_compile_options(mtable PRIVATE
        -std=c++2c
        -freflection
        -fexpansion-statements
        -Wall -Wextra -Wpedantic
        -stdlib=libc++
)

target_link_options(mtable PRIVATE
        -Wl,-rpath,/home/andrei/Desktop/clang26/build/lib/x86_64-unknown-linux-gnu
)

# ---- Optional: print compiler info ----
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Compiler version: ${CMAKE_CXX_COMPILER_VERSION}")