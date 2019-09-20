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

set(CENTAURUS_LIB_SRC src/core/ATN.cpp src/core/CharClass.cpp src/core/CodeGenEM64T.cpp src/core/CompositeATN.cpp src/core/Util.cpp src/core/Grammar.cpp)
set(CENTAURUS_DRIVER_SRC src/tool/main.cpp)
set(CENTAURUS_PYLIB_SRC src/pydll/PyCentaurus.cpp)

add_definitions("-DASMJIT_STATIC")
if(CMAKE_SYSTEM_NAME STREQUAL "Windows" OR CMAKE_SYSTEM_NAME STREQUAL "CYGWIN")
    add_definitions("-DCENTAURUS_BUILD_WINDOWS")
    add_definitions("-DCENTAURUS_PROJECT_DIR=\"${CMAKE_SOURCE_DIR}\"")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_definitions("-DCENTAURUS_BUILD_LINUX")
endif()

add_library(libcentaurus)
set_target_properties(libcentaurus PROPERTIES PREFIX "")
target_link_libraries(libcentaurus asmjit)
target_include_directories(libcentaurus PRIVATE asmjit/src)
target_sources(libcentaurus PRIVATE ${CENTAURUS_LIB_SRC})

add_library(libpycentaurus SHARED)
set_target_properties(libpycentaurus PROPERTIES PREFIX "")
target_link_libraries(libpycentaurus asmjit)
target_include_directories(libpycentaurus PRIVATE asmjit/src src/core src/pydll ${CMAKE_CURRENT_BINARY_DIR})
target_sources(libpycentaurus PRIVATE ${CENTAURUS_LIB_SRC} ${CENTAURUS_PYLIB_SRC})
generate_export_header(libpycentaurus BASE_NAME pycentaurus)

add_executable(centaurus ${CENTAURUS_DRIVER_SRC})
target_include_directories(centaurus PRIVATE asmjit/src src/core src/pydll)
target_link_libraries(centaurus libcentaurus)

add_executable(dblp_std benchmarks/cpp/dblp_std.cpp)
target_include_directories(dblp_std PRIVATE asmjit/src src/core)
target_link_libraries(dblp_std libcentaurus)

add_executable(dblp_opt benchmarks/cpp/dblp_opt.cpp pugixml/src/pugixml.cpp)
target_include_directories(dblp_opt PRIVATE asmjit/src src/core pugixml/src)
target_link_libraries(dblp_opt libcentaurus)

add_executable(citylots_std benchmarks/cpp/citylots_std.cpp)
target_include_directories(citylots_std PRIVATE asmjit/src src/core)
target_link_libraries(citylots_std libcentaurus)

add_executable(citylots_opt2 benchmarks/cpp/citylots_opt2.cpp)
target_include_directories(citylots_opt2 PRIVATE asmjit/src src/core rapidjson/include)
target_link_libraries(citylots_opt2 libcentaurus)

add_executable(dry benchmarks/cpp/dry.cpp)
target_include_directories(dry PRIVATE asmjit/src src/core)
target_link_libraries(dry libcentaurus)

add_executable(pugixml_load_file benchmarks/cpp/pugixml_load_file.cpp pugixml/src/pugixml.cpp)
target_include_directories(pugixml_load_file PRIVATE pugixml/src)

add_executable(pugixml_load_mmap benchmarks/cpp/pugixml_load_mmap.cpp pugixml/src/pugixml.cpp)
target_include_directories(pugixml_load_mmap PRIVATE pugixml/src benchmarks/cpp/)

add_executable(rapidjson_load_file benchmarks/cpp/rapidjson_load_file.cpp)
target_include_directories(rapidjson_load_file PRIVATE rapidjson/include)

add_executable(rapidjson_load_mmap benchmarks/cpp/rapidjson_load_mmap.cpp)
target_include_directories(rapidjson_load_mmap PRIVATE rapidjson/include benchmarks/cpp/)

add_executable(rapidjson_read benchmarks/cpp/rapidjson_read.cpp)
target_include_directories(rapidjson_read PRIVATE rapidjson/include benchmarks/cpp/)


if(NOT MSVC)
    add_subdirectory(tests)
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
