cmake_minimum_required(VERSION 3.16)

project(minimal_qt_app)

set(CMAKE_CXX_STANDARD 26)

find_package(Qt6 REQUIRED COMPONENTS Widgets)



add_executable(minimal_qt_app main.cpp)

target_include_directories(minimal_qt_app PRIVATE
        /home/andrei/Desktop/clang26/build/include/c++/v1
        /home/andrei/Desktop/clang26/build/include/x86_64-unknown-linux-gnu/c++/v1
)

target_link_directories(minimal_qt_app PRIVATE
        /home/andrei/Desktop/clang26/build/lib/x86_64-unknown-linux-gnu
)

target_link_libraries(minimal_qt_app PRIVATE
        Qt6::Widgets
        c++
        c++abi
        unwind
)

target_compile_options(minimal_qt_app PRIVATE
        -std=c++2c
        -stdlib=libc++
        -freflection
        -fexpansion-statements
)

target_link_options(minimal_qt_app PRIVATE
        -stdlib=libc++
        -Wl,-rpath,/home/andrei/Desktop/clang26/build/lib/x86_64-unknown-linux-gnu
)


target_link_libraries(minimal_qt_app
    PRIVATE Qt6::Widgets
)
