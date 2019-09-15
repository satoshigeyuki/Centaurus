add_library(UnitTest1 SHARED NFATest.cpp DFATest.cpp LDFATest.cpp unittest1.cpp JITTest.cpp CodeGenTest.cpp)
target_link_libraries(UnitTest1 libcentaurus)
target_include_directories(UnitTest1 PRIVATE ../MsTestEmulator/include ../src/core ../asmjit/src ../tests)
add_custom_target(mstest COMMAND bash ${PROJECT_SOURCE_DIR}/MsTestEmulator/scripts/run.sh -d $<TARGET_FILE:testdriver> $<TARGET_FILE:UnitTest1> DEPENDS UnitTest1 testdriver)
