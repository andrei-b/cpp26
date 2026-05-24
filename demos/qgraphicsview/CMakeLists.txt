cmake_minimum_required(VERSION 3.16)

project(qgraphics_string_slots_example LANGUAGES CXX)

# Adjust these two paths for your local setup.
set(CLANG21_ROOT /opt/clang-p2996/ CACHE PATH "Reflection-enabled Clang root")
set(CMAKE_PREFIX_PATH "/home/andrei/qtbase/build-clang" CACHE PATH "Qt6 prefix")

set(CMAKE_C_COMPILER ${CLANG21_ROOT}/bin/clang)
set(CMAKE_CXX_COMPILER ${CLANG21_ROOT}/bin/clang++)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(qgraphics_string_slots main.cpp)

target_include_directories(qgraphics_string_slots PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CLANG21_ROOT}/include/c++/v1
    ${CLANG21_ROOT}/include/x86_64-unknown-linux-gnu/c++/v1
)

target_link_directories(qgraphics_string_slots PRIVATE
    ${CLANG21_ROOT}/lib/x86_64-unknown-linux-gnu
)

target_compile_options(qgraphics_string_slots PRIVATE
    -std=c++2c
    -stdlib=libc++
    -freflection
    -fparameter-reflection
    -fexpansion-statements
)

target_link_options(qgraphics_string_slots PRIVATE
    -stdlib=libc++
    -Wl,-rpath,${CLANG21_ROOT}/lib/x86_64-unknown-linux-gnu
)

target_link_libraries(qgraphics_string_slots PRIVATE
    Qt6::Widgets
    c++
    c++abi
    unwind
)
