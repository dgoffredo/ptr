cmake_minimum_required(VERSION 3.2...3.28)

project(ptr)

set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(ptr INTERFACE)
target_include_directories(ptr INTERFACE include)
target_compile_options(ptr INTERFACE "-Wall;-Wextra;-pedantic;-Werror;-fsanitize=address;-fsanitize=undefined")
target_link_options(ptr INTERFACE "-fsanitize=address;-fsanitize=undefined")

add_executable(ptr_test test.cpp)
target_link_libraries(ptr_test ptr)
