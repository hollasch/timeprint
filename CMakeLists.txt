cmake_minimum_required(VERSION 3.2...3.27.7)

project (timeprint LANGUAGES CXX)

set (CMAKE_CXX_STANDARD          17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS        OFF)

add_executable (timeprint timeprint.cpp)
