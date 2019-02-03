cmake_minimum_required(VERSION 3.1)
enable_testing()

project(Centaurus CXX)

include(CheckCXXSourceCompiles)
include(GenerateExportHeader)

set(ASMJIT_STATIC TRUE)
set(ASMJIT_BUILD_ARM FALSE)
set(ASMJIT_BUILD_X86 TRUE)
set(ASMJIT_BUILD_TEST FALSE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -march=native -ftree-vectorize")

add_subdirectory(asmjit)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-switch -Wno-sign-compare -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-unused-result -Wno-bool-compare -std=c++11 -fvisibility=hidden")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

set(CENTAURUS_LIB_SRC src/ATN.cpp src/CharClass.cpp src/CodeGenEM64T.cpp src/CompositeATN.cpp src/Util.cpp src/Stage1Runner.cpp src/Stage2Runner.cpp src/Stage3Runner.cpp src/Grammar.cpp)
set(CENTAURUS_DRIVER_SRC src/main.cpp)
set(CENTAURUS_LIB_HDR src/ATN.hpp src/CharClass.hpp src/CodeGenEM64T.hpp src/CompositeATN.hpp src/DFA.hpp src/Exception.hpp src/Grammar.hpp src/Identifier.hpp src/LookaheadDFA.hpp src/NFA.hpp src/Stream.hpp src/Util.hpp)
set(CENTAURUS_PYLIB_SRC vc/PyCentaurus/PyCentaurus.cpp)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_definitions("-DCENTAURUS_BUILD_WINDOWS")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_definitions("-DCENTAURUS_BUILD_LINUX")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Cygwin")
    add_definitions("-DCENTAURUS_BUILD_WINDOWS")
endif()

add_library(libcentaurus)
set_target_properties(libcentaurus PROPERTIES PREFIX "")
target_link_libraries(libcentaurus asmjit)
target_include_directories(libcentaurus PRIVATE asmjit/src vc)
target_sources(libcentaurus PRIVATE ${CENTAURUS_LIB_SRC})

add_library(libpycentaurus SHARED)
set_target_properties(libpycentaurus PROPERTIES PREFIX "")
target_link_libraries(libpycentaurus asmjit)
target_include_directories(libpycentaurus PRIVATE asmjit/src src vc ${CMAKE_CURRENT_BINARY_DIR})
target_sources(libpycentaurus PRIVATE ${CENTAURUS_LIB_SRC} ${CENTAURUS_PYLIB_SRC})
generate_export_header(libpycentaurus BASE_NAME pycentaurus)

add_executable(centaurus ${CENTAURUS_DRIVER_SRC})
target_include_directories(centaurus PRIVATE asmjit/src vc)
target_link_libraries(centaurus libcentaurus)

add_executable(bench1 benchmarks/bench1.cpp)
target_include_directories(bench1 PRIVATE asmjit/src vc src)
target_link_libraries(bench1 libcentaurus)

add_executable(bench2 benchmarks/bench2.cpp)
target_include_directories(bench2 PRIVATE asmjit/src vc src)
target_link_libraries(bench2 libcentaurus)

if(NOT MSVC)
    add_subdirectory(UnitTest1)
	add_subdirectory(MsTestEmulator)

    add_test(NAME LoadGrammarFile COMMAND bash MsTestEmulator/scripts/run.sh -m LoadGrammarFile -d $<TARGET_FILE:testdriver> $<TARGET_FILE:UnitTest1> WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
    add_test(NAME test_wet_parser_mp COMMAND python3 -m unittest unittest1.Test_unittest1.test_wet_parser_mp WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/PythonApp1)
    set_tests_properties(test_wet_parser_mp PROPERTIES ENVIRONMENT CENTAURUS_DL_PATH=${CMAKE_CURRENT_BINARY_DIR})
    add_test(NAME test_wet_parser COMMAND python3 -m unittest unittest1.Test_unittest1.test_wet_parser WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/PythonApp1)
    set_tests_properties(test_wet_parser PROPERTIES ENVIRONMENT CENTAURUS_DL_PATH=${CMAKE_CURRENT_BINARY_DIR})
    add_test(NAME test_dry_parser COMMAND python3 -m unittest unittest1.Test_unittest1.test_dry_parser WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/PythonApp1)
    set_tests_properties(test_dry_parser PROPERTIES ENVIRONMENT CENTAURUS_DL_PATH=${CMAKE_CURRENT_BINARY_DIR})
    add_test(NAME measure_performance COMMAND python3 -m unittest unittest1.Test_unittest1.measure_performance WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/PythonApp1)
    set_tests_properties(measure_performance PROPERTIES ENVIRONMENT CENTAURUS_DL_PATH=${CMAKE_CURRENT_BINARY_DIR})
endif()
