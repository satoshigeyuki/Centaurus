add_library(UnitTest1 SHARED NFATest.cpp DFATest.cpp LDFATest.cpp unittest1.cpp)
target_link_libraries(UnitTest1 libcentaurus)
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-z,defs")
target_include_directories(UnitTest1 PRIVATE ../MsTestEmulator/include ../src)
