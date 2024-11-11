cmake_minimum_required(VERSION 3.2)

project(ptr)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(ptr INTERFACE)
target_include_directories(ptr INTERFACE include)

add_executable(ptr_test test.cpp)
target_link_libraries(ptr_test ptr)
