cmake_minimum_required(VERSION 3.2...3.28)

project(ptr)

# set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# tracing compilation
if (CMAKE_CXX_COMPILER_ID MATCHES "^(Apple)?Clang$")
    add_compile_options("-ftime-trace")
    add_link_options("-ftime-trace")
endif()

add_library(ptr INTERFACE)
target_include_directories(ptr INTERFACE include)
target_compile_options(ptr INTERFACE "-Wall;-Wextra;-pedantic;-Werror;-fno-omit-frame-pointer;-fsanitize=address;-fsanitize=undefined")
target_link_options(ptr INTERFACE "-fsanitize=address;-fsanitize=undefined")

add_subdirectory(test)
