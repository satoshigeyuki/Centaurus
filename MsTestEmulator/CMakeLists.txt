cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++11 -Os")

add_executable(testdriver runner/main.cpp)
target_link_libraries(testdriver PRIVATE dl)
