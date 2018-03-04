add_library(UnitTest1 SHARED NFATest.cpp DFATest.cpp LDFATest.cpp unittest1.cpp)

target_include_directories(UnitTest1 PRIVATE ../MsTestEmulator/include ../src)
