cmake_minimum_required(VERSION 3.22)
project(signalslot LANGUAGES CXX)

set(CLANG21_ROOT /opt/clang-p2996/)

set(CMAKE_C_COMPILER ${CLANG21_ROOT}/bin/clang)
set(CMAKE_CXX_COMPILER ${CLANG21_ROOT}/bin/clang++)

set(CMAKE_BUILD_RPATH
        "${CLANG21_ROOT}/lib"
        "${CLANG21_ROOT}/lib/x86_64-unknown-linux-gnu"
)

set(CMAKE_INSTALL_RPATH
        "${CLANG21_ROOT}/lib"
        "${CLANG21_ROOT}/lib/x86_64-unknown-linux-gnu"
)

add_executable(pipeline src/main.cpp
        include/i_calling_queue.h)

target_include_directories(pipeline PRIVATE
        ${CLANG21_ROOT}/include/c++/v1
        ${CLANG21_ROOT}/include/x86_64-unknown-linux-gnu/c++/v1
        include
)

target_link_directories(pipeline PRIVATE
        ${CLANG21_ROOT}/lib/x86_64-unknown-linux-gnu
)

target_link_libraries(pipeline PRIVATE
        c++
        c++abi
        unwind
)

target_compile_features(pipeline PRIVATE cxx_std_26)

target_compile_options(pipeline PRIVATE
        -std=c++2c
        -stdlib=libc++
        -freflection
        -fparameter-reflection
        -fexpansion-statements
)

target_link_options(pipeline PRIVATE
        -stdlib=libc++
        -Wl,-rpath,${CLANG21_ROOT}/lib/x86_64-unknown-linux-gnu
)
