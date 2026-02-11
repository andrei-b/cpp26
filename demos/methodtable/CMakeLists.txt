cmake_minimum_required(VERSION 4.1)
project(untitled1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(methodtable main.cpp)

# Enable C++26 reflection (GCC trunk)
target_compile_options(methodtable PRIVATE
        -freflection
)
