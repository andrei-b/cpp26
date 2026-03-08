cmake_minimum_required(VERSION 3.28)

project(invokable LANGUAGES CXX)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
# ---- Require C++2c (C++26 draft) ----
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- Create executable ----
add_executable(invokable main.cpp)


target_include_directories(invokable PRIVATE
        /home/andrei/Desktop/clang26/build/include/c++/v1
        /home/andrei/Desktop/clang26/build/include/x86_64-unknown-linux-gnu/c++/v1
)

target_link_directories(invokable PRIVATE
        /home/andrei/Desktop/clang26/build/lib/x86_64-unknown-linux-gnu
)

# link libc++ runtime
target_link_libraries(invokable PRIVATE
        c++
        c++abi
        unwind
)


# ---- Extra warnings (recommended) ----
target_compile_options(invokable PRIVATE
        -std=c++2c
        -freflection
        -fexpansion-statements
        -Wall -Wextra -Wpedantic
        -stdlib=libc++
)

target_link_options(invokable PRIVATE
        -Wl,-rpath,/home/andrei/Desktop/clang26/build/lib/x86_64-unknown-linux-gnu
)

# ---- Optional: print compiler info ----
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Compiler version: ${CMAKE_CXX_COMPILER_VERSION}")