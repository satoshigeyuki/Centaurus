Centaurus
========

Centaurus is a performance-oriented LL(\*)-S parser generator.

## Supported platforms

* Linux x86-64 g++
The code generator will use the System V AMD64 ABI for function calls,
and POSIX IPC (message queues, semaphores and shared memory) for master-slave communication.
* Windows x86-64 MSVC
The code generator will use the Microsoft AMD64 ABI for function calls,
and Win32 IPC (named pipes, semaphores and shared memory) for master-slave communication.

## What is with the name?

Centaurus is a bright constellation low down in the southern sky (in Japan).
The alpha star of the constellation (*Alpha Centauri*, -0.1 mv) is also known as the closest star to Earth, 4.37 light years away.
The constellation was inspired from an imaginary animal which is a chimera between a human and a horse.

The name Centaurus of the software comes from the fact that it is a chimera between a parser and a lexer, like the imaginary animal.
The software is also seen as a chimera of PEG based methods (e.g. Packrat parsing) and LL(k) based methods,
being the first and only software (as of 2018) to generate a *scannerless* and *deterministic* parser.

The naming also follows the tradition in the field of parser development that a parser has to be named after a four-legged (and often domesticated) animal.
Yacc (LALR) is taken from yak, Bison (LALR/GLR) and Elkhound (GLR) from the animal with the same names and ANTLR (ALL(*)) from antlers (deer horns).

## Build instructions

### Linux/Cygwin (GCC)

```CMake (>=3.1)``` along with ```g++ (>=4.9)``` is required to build the software.

#### Release build

```
 $ git clone https://gitlab.eidos.ic.i.u-tokyo.ac.jp/ihara/Centaurus.git
 $ cd Centaurus
 $ git submodule update --init
 $ mkdir build && cd build
 $ cmake .. -DCMAKE_BUILD_TYPE=Release
```

### Visual Studio

The software is shipped with a solution file compatible with Visual Studio 2017.
VS2017 has to have the following components installed:

 * Basic C++ support
 * MSVC v141 toolset
 * C++ Unit Testing Framework

## Running custom programs

### C++



### Python

As for the interpreter, currently only Python 2 is supported. The software depends on ```ctypes``` to interface the Python script with the shared libraries compiled from C++.
The Python library searches for the shared libraries within the directory specified in the environment variable ```CENTAURUS_DL_PATH```

A Python program has to know where 