cmake_minimum_required(VERSION 3.2)

project (timeprint LANGUAGES CXX)
add_executable (timeprint timeprint.cpp)

target_compile_features(timeprint PUBLIC cxx_std_17)
