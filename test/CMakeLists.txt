add_executable(ptr_test
    breathing.cpp
    test.cpp)
target_link_libraries(ptr_test ptr)
target_include_directories(ptr_test PRIVATE ./)
