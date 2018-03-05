cmake_minimum_required(VERSION 3.1)

project(Centaurus CXX)

set(ASMJIT_STATIC TRUE)
set(ASMJIT_BUILD_ARM FALSE)
set(ASMJIT_BUILD_X86 TRUE)
set(ASMJIT_BUILD_TEST FALSE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

add_subdirectory(asmjit)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-switch -Wno-sign-compare -std=c++11 -fvisibility=hidden")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Os")

set(CENTAURUS_LIB_SRC src/ATN.cpp src/CharClass.cpp src/CodeGenEM64T.cpp src/CompositeATN.cpp src/Util.cpp)
set(CENTAURUS_DRIVER_SRC src/main.cpp)
set(CENTAURUS_LIB_HDR src/ATN.hpp src/CharClass.hpp src/CodeGenEM64T.hpp src/CompositeATN.hpp src/DFA.hpp src/Exception.hpp src/Grammar.hpp src/Identifier.hpp src/LookaheadDFA.hpp src/NFA.hpp src/Stream.hpp src/Util.hpp)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    add_definitions("-DCENTAURUS_BUILD_WINDOWS")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    add_definitions("-DCENTAURUS_BUILD_LINUX")
endif()

add_library(libcentaurus)
set_target_properties(libcentaurus PROPERTIES PREFIX "")
target_link_libraries(libcentaurus asmjit)
target_include_directories(libcentaurus PRIVATE asmjit/src)
target_sources(libcentaurus PRIVATE ${CENTAURUS_LIB_SRC})

add_executable(centaurus ${CENTAURUS_DRIVER_SRC})
target_link_libraries(centaurus libcentaurus)

if(NOT MSVC)
    add_subdirectory(UnitTest1)
	add_subdirectory(MsTestEmulator)
endif()
