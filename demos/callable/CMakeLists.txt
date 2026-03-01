cmake_minimum_required(VERSION 3.28)

project(callable LANGUAGES CXX)

# ---- Require C++2c (C++26 draft) ----
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- Create executable ----
add_executable(callable main.cpp)

# ---- Extra warnings (recommended) ----
target_compile_options(callable PRIVATE
        -std=c++2c
        -freflection
        -Wall -Wextra -Wpedantic
)

# ---- Enable reflection if using GCC trunk ----
option(ENABLE_REFLECTION "Enable experimental reflection" ON)

if(ENABLE_REFLECTION)
    target_compile_options(callable PRIVATE -freflection)
endif()

# ---- Optional: print compiler info ----
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Compiler version: ${CMAKE_CXX_COMPILER_VERSION}")