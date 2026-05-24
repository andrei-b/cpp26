cmake_minimum_required(VERSION 3.22)
project(signalslot LANGUAGES CXX)

add_executable(sensor_pipeline src/main.cpp)
target_include_directories(sensor_pipeline PRIVATE include)
target_compile_features(sensor_pipeline PRIVATE cxx_std_26)
