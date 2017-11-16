cmake_minimum_required(VERSION 2.8)

project(centaurus)

add_definitions("-Wall -Wextra -std=c++11 -O2")
add_executable(centaurus main.cpp atn.cpp util.cpp charclass.cpp)
