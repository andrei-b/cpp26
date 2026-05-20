cmake_minimum_required(VERSION 3.16)

project(umlwithreflection)

set(CLANG21_ROOT /opt/clang-p2996/)

set(CMAKE_C_COMPILER ${CLANG21_ROOT}/bin/clang)
set(CMAKE_CXX_COMPILER ${CLANG21_ROOT}/bin/clang++)

set(CMAKE_CXX_STANDARD 26)

set(CMAKE_PREFIX_PATH "/home/andrei/qtbase/build-clang")

find_package(Qt6 REQUIRED COMPONENTS Widgets)


set(CMAKE_BUILD_RPATH
        "${CLANG21_ROOT}/lib"
        "${CLANG21_ROOT}/lib/x86_64-unknown-linux-gnu"
)

set(CMAKE_INSTALL_RPATH
        "${CLANG21_ROOT}/lib"
        "${CLANG21_ROOT}/lib/x86_64-unknown-linux-gnu"
)

add_executable(umlwithreflection main.cpp)

target_include_directories(umlwithreflection PRIVATE
        ${CLANG21_ROOT}/include/c++/v1
        ${CLANG21_ROOT}/include/x86_64-unknown-linux-gnu/c++/v1
)

target_link_directories(umlwithreflection PRIVATE
        ${CLANG21_ROOT}/lib/x86_64-unknown-linux-gnu
)

target_link_libraries(umlwithreflection PRIVATE
        Qt6::Widgets
        c++
        c++abi
        unwind
)

target_compile_options(umlwithreflection PRIVATE
        -std=c++2c
        -stdlib=libc++
        -freflection
        -fparameter-reflection
        -fexpansion-statements
)

target_link_options(umlwithreflection PRIVATE
        -stdlib=libc++
        -Wl,-rpath,${CLANG21_ROOT}/lib/x86_64-unknown-linux-gnu
)


target_link_libraries(umlwithreflection
    PRIVATE Qt6::Widgets
)
