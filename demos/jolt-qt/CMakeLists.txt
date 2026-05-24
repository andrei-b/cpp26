cmake_minimum_required(VERSION 3.20)
project(jolt_qgraphics_reflected_slots LANGUAGES CXX)

# Adjust this to your reflection-enabled clang build if needed.
set(CLANG21_ROOT /opt/clang-p2996 CACHE PATH "Reflection-enabled clang root")
set(CLANG21_LIBDIR ${CLANG21_ROOT}/lib/x86_64-unknown-linux-gnu)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Keep all C++ targets (including FetchContent dependencies) on libc++.
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-stdlib=libc++>)
add_link_options(-stdlib=libc++)

include(FetchContent)

FetchContent_Declare(
    JoltPhysics
    URL https://github.com/jrouwe/JoltPhysics/archive/refs/tags/v5.3.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_SUBDIR Build
)


find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Jolt CONFIG QUIET)

if(NOT Jolt_FOUND)
    message(STATUS "Jolt package not found. Falling back to FetchContent (JoltPhysics v5.3.0).")
    FetchContent_MakeAvailable(JoltPhysics)

    # Jolt's source build usually defines `Jolt`; create namespaced alias expected below.
    if(TARGET Jolt AND NOT TARGET Jolt::Jolt)
        add_library(Jolt::Jolt ALIAS Jolt)
    endif()
endif()

if(NOT TARGET Jolt::Jolt)
    message(FATAL_ERROR "Jolt was not found and fallback build did not provide target Jolt::Jolt")
endif()


add_executable(jolt_qgraphics_reflected_slots
    src/main.cpp
)

set_target_properties(jolt_qgraphics_reflected_slots PROPERTIES
    BUILD_RPATH ${CLANG21_LIBDIR}
    INSTALL_RPATH ${CLANG21_LIBDIR}
)

target_include_directories(jolt_qgraphics_reflected_slots PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CLANG21_ROOT}/include/c++/v1
    ${CLANG21_ROOT}/include/x86_64-unknown-linux-gnu/c++/v1
)

target_compile_options(jolt_qgraphics_reflected_slots PRIVATE
    -std=c++2c
    -freflection
    -fparameter-reflection
    -fexpansion-statements
)


target_link_libraries(jolt_qgraphics_reflected_slots PRIVATE
    Qt6::Widgets
    Jolt::Jolt
    c++
    c++abi
    unwind
)
