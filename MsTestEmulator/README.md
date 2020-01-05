# MsTestEmulator
MsTestEmulator is intended to serve as a drop-in replacement for Microsoft C++ Unit Test Framework,
enabling you to port platform-agnostic C++ tests, without any modification, from Visual Studio to Linux G++.
## When do you find this software useful? Why not use Google Test?
Suppose you have developed a C++ program with Visual Studio,
mostly comprised of platform-agnostic code (i.e. mostly dependent on CRT functions with least calls to POSIX functions
, which could be replaced with equivalent Win32/COM calls).
MsTestEmulator takes you to the actual test runs without installing test adaptors to third-party testing frameworks like Google Test,
by making use of the embedded C++ Unit Test Framework.
You can just write the tests on the Unit Test Framework and run them on Linux later when portability becomes a requirement.
## Requirements
MsTestEmulator only supports Linux g++. Support for Clang/LLVM is not yet planned.
In the first place of course, the test code itself has to be compiled by g++ and run on Linux.

Apart from g++ and objdump, the runner script currently depends on bash.
## How it works
MsTestEmulator consists of three components, the headers (inside include/), the runner (inside runner/) and the testing script (inside scripts/).
The headers (CppUnitTest.h and SDKDDKVer.h) are to be included by the test code at compile time,
while the other two components are combined at runtime to act as a testing platform.
### Compile time
Using the headers, your test code is packed into a single shared object file (.so),
with all the external references resolved.
The object file is more of an executable file with symbols exported for inspection by the test runner,
rather than a shared library.
### Runtime
When invoked from test drivers like CTest, the runner script invokes objdump several times to extract all the entry points from the DSO.
The runner binary, compiled separately from the shared object and invoked by the runner script,
will then use dlsym() to run each test.
## Usage
1. Build your test code and the runner binary in any way you like.
You have to pay attention to the linking phase so that there are no unresolved references in the resulting shared object.
For g++, two options ```-Wl,-z,defs``` and ```-fvisibility=hidden``` are recommended, in additions to normal options like ```-fPIC``` and ```-shared```.
```-Wl,-z,defs``` demands that the linker emits an error when there are unresolved symbols.
Unresolved symbols left at runtime will be reported by the runtime binary.
```-fvisibility=hidden``` orders the linker should include the least number of symbols in the DSO,
eventually speeding up enumerating the tests.
2. Run the test script.
The test script can be run at least with two arguments, the path to runner binary (```-d```) and the path to shared object.
In this mode, all the tests are run in some random order.
You can additionally specify ```-i``` to invoke the runner script in interactive mode,
where you are prompted to select the tests listed on the console.
