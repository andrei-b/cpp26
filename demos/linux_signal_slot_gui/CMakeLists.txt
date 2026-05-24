cmake_minimum_required(VERSION 3.24)
project(linux_signal_slot_gui LANGUAGES CXX)

set(CLANG21_ROOT /opt/clang-p2996/)

set(CMAKE_C_COMPILER ${CLANG21_ROOT}/bin/clang)
set(CMAKE_CXX_COMPILER ${CLANG21_ROOT}/bin/clang++)


set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_BUILD_RPATH
        "${CLANG21_ROOT}/lib"
        "${CLANG21_ROOT}/lib/x86_64-unknown-linux-gnu"
)

set(CMAKE_INSTALL_RPATH
        "${CLANG21_ROOT}/lib"
        "${CLANG21_ROOT}/lib/x86_64-unknown-linux-gnu"
)


find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)

add_executable(signal_slot_gui main.cpp)

target_include_directories(signal_slot_gui PRIVATE
        ${CLANG21_ROOT}/include/c++/v1
        ${CLANG21_ROOT}/include/x86_64-unknown-linux-gnu/c++/v1
)

target_link_directories(signal_slot_gui PRIVATE
        ${CLANG21_ROOT}/lib/x86_64-unknown-linux-gnu
)

target_link_libraries(signal_slot_gui PRIVATE PkgConfig::GTK4
        c++
        c++abi
        unwind
)
target_compile_options(signal_slot_gui PRIVATE -Wall -Wextra -Wpedantic -std=c++2c -stdlib=libc++ -freflection -fparameter-reflection -fexpansion-statements)
