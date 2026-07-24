cmake_minimum_required(VERSION 3.20)
set(CLANG21_ROOT "" CACHE PATH "Reflection-enabled clang root")
if(NOT DEFINED CMAKE_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER ${CLANG21_ROOT}/bin/clang++ CACHE FILEPATH "C++ compiler")
endif()

project(dump_json LANGUAGES CXX)

# Adjust this to your reflection-enabled clang build if needed.
set(CLANG21_LIBDIR ${CLANG21_ROOT}/lib/x86_64-unknown-linux-gnu)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Keep all C++ targets (including FetchContent dependencies) on libc++.
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-stdlib=libc++>)
add_link_options(-stdlib=libc++)

include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

add_executable(dump_json main.cpp)

set_target_properties(dump_json PROPERTIES
    BUILD_RPATH ${CLANG21_LIBDIR}
    INSTALL_RPATH ${CLANG21_LIBDIR}
)

target_include_directories(dump_json PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CLANG21_ROOT}/include/c++/v1
        ${CLANG21_ROOT}/include/x86_64-unknown-linux-gnu/c++/v1
)

target_compile_options(dump_json PRIVATE
    -std=c++2c
)

target_link_libraries(dump_json PRIVATE nlohmann_json::nlohmann_json)

target_compile_options(dump_json PRIVATE
        -std=c++2c
        -freflection
        -fparameter-reflection
        -fexpansion-statements
)


target_link_libraries(dump_json PRIVATE
        c++
        c++abi
        unwind
)
