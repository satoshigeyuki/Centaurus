cmake_minimum_required(VERSION 2.8)

project(Centaurus CXX)

set(ASMJIT_STATIC TRUE)
set(ASMJIT_BUILD_ARM FALSE)
set(ASMJIT_BUILD_X86 TRUE)
set(ASMJIT_BUILD_TEST FALSE)

add_subdirectory(asmjit)

add_definitions("-DASMJIT_STATIC")

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-switch -Wno-sign-compare -std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-Os")

set(CENTAURUS_MAIN_SRC src/ATN.cpp src/CharClass.cpp src/CodeGenEM64T.cpp src/CompositeATN.cpp src/Util.cpp)
set(CENTAURUS_DRIVER_SRC src/main.cpp)

add_executable(centaurus ${CENTAURUS_MAIN_SRC} ${CENTAURUS_DRIVER_SRC})
target_include_directories(centaurus PRIVATE asmjit/src)
target_link_libraries(centaurus asmjit)
