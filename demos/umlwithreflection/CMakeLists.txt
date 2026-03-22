cmake_minimum_required(VERSION 3.16)

project(umlwithreflection)

set(CMAKE_CXX_STANDARD 26)

#CMAKE_PREFIX_PATH=/opt/qt-clang26
set(CMAKE_PREFIX_PATH "/opt/qt-clang26")

find_package(Qt6 REQUIRED COMPONENTS Widgets)



add_executable(umlwithreflection main.cpp)

target_include_directories(umlwithreflection PRIVATE
        /home/andrei/Desktop/clang26/build/include/c++/v1
        /home/andrei/Desktop/clang26/build/include/x86_64-unknown-linux-gnu/c++/v1
)

target_link_directories(umlwithreflection PRIVATE
        /home/andrei/Desktop/clang26/build/lib/x86_64-unknown-linux-gnu
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
        -fexpansion-statements
)

target_link_options(umlwithreflection PRIVATE
        -stdlib=libc++
        -Wl,-rpath,/home/andrei/Desktop/clang26/build/lib/x86_64-unknown-linux-gnu
)


target_link_libraries(umlwithreflection
    PRIVATE Qt6::Widgets
)
