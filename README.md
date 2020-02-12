Centaurus
========

Centaurus is a performance-oriented LL(\*)-S parser generator.

## How to use

Unlike famous parser generators like Bison and ANTLR, Centaurus is not packaged as a standalone executable. Instead of generating the parser source code from the grammar ahead of runtime (AOT), Centaurus directly generates the parser code at runtime (JIT). This operation principle requires Centaurus to be linked to the user program as a library.

To generate a parser with Centaurus, the user program simply has to pass a grammar in the form of string to the library. Centaurus will generate the executable parser code on-the-fly which is returned encapsulated in an opaque object. The actual parsing could be performed by attaching semantic actions and handing the input data to this object.

Build instructions for the library could be found below.

## Operating principle and key features

### JIT parser generation

Centaurus compiles a CFG-style grammar written in EBNF into an executable parser code at runtime. The direct translation from the grammar to the executable code enables the loophole optimizer to utilize the structural information of the grammar, which is lost when the grammar is translated into source code.

One of the loophole optimization techniques implemented in the current version is efficient regex matching using the SSE4.2 instruction set. When instructed, the JIT code generator will employ the PCMPISTRI instruction to match against an indefinite repetition of character classes, which is denoted using Kleene stars. This optimization will enable the generated program to read up to 16 characters in a single instruction, as opposed to one when the optimization does not take place.

### Parallel reduction

The generated parser itself does not get any meaningful job done; all it does is test if the input data matches the grammar specification. To actually parse the data, semantic actions must be attached to the grammar, which stipulate how the data are extracted and converted so the user program could utilize them.

With Centaurus, the semantic actions are defined in the form of callbacks within the user program, which are invoked from within the library as the parsing process goes on.

The implementation of callbacks quite resembles that of Bison and ANTLR, in that they are given a list of semantic values associated with each symbol on the RHS of a reduction rule, and they return the semantic value associated with the LHS symbol.

However, Centaurus differs from Bison and ANTLR in that these callbacks are invoked in parallel to improve the parsing performance. Because the parallelization happens within the runtime library in a transparent manner, the callbacks could be written exactly as they are written for serial reduction, except that they must not have any side effects for obvious reasons.

Centaurus is also capable of parallelizing the reduction workload across multiple processes instead of threads. This feature could be beneficial for interpreter platforms with GIL (global interpreter lock), including CPython and Ruby MRI, though only CPython is currently supported as an interpreter platform.

## Supported platforms

Currently, Centaurus only supports Linux running on an AMD64 processor with SSE4.2, and it has to be compiled with g++ (Clang is not supported). The library code is actually designed to work with Windows MSVC and Cygwin g++, but the build system needs further modification to be compatible with these platforms.

The generated parser code conforms to the System V AMD64 ABI, while POSIX IPC (message queues, semaphores and shared memory) is utilized by the runtime library for communication between the workers.

## Build instructions

### Linux g++

```CMake (>=3.1)``` along with ```g++ (>=4.9)``` is required to build the software.

```
 $ git clone https://github.com/satoshigeyuki/Centaurus.git
 $ cd Centaurus
 $ git submodule update --init --recursive
 $ mkdir build && cd build
 $ cmake .. -DCMAKE_BUILD_TYPE=Release
 $ make && make install
```

## Tutorial

### Tutorial 1. Build a calculator

In this tutorial, you will learn how to make a simple C++ calculator using Centaurus.

#### Step 1. Write the grammar

An example grammar for the calculator is shown below:

```
grammar CALC;

INPUT : EXPR ;
EXPR : TERM '+' EXPR | TERM ;
TERM : FACT '*' TERM | FACT ;
FACT : /[0-9]+/ | '(' EXPR ')' ;
```

There are few things to note in this example.

+ The grammar must be compatible with the LL grammar class, which means there cannot be any left recursion.
+ The scanner definition is merged into the grammar, because we don't have a scanner. The grammar will be compiled into a character-level PDA which parses the input one character at a time.
+ The first line denotes the name of the grammar, but it does not have any meaning at this moment. The directive is optional.

#### Step 2. Write the semantic actions

Once we defined the grammar, we have to write the semantic action to associate with each rule in the grammar. The semantic actions must be defined within the user program, not the grammar. This is because the grammar does not go through the host compiler, unlike Bison; it is only processed by the parser generator which only understands the grammar part.

Each semantic action has the following signature:

```c++
void *func(const SymbolContext<char>& ctx);
```

The only parameter, `ctx`, contains all the necessary information to implement the action, including the semantic value of each RHS symbol and the string representation of LHS/RHS symbols.

The function returns a `void` pointer which shall wrap the semantic value in an opaque manner. This could typically be a pointer to a user-defined class casted to `void *`.

The semantic actions for the calculator example is shown below:

```c++
#include <string>
#include <Context.hpp>

using namespace Centaurus;

struct Number
{
    int num;
    Number(int num) : num(num) {}
    Number(const std::string& str) : num(std::stoi(str)) {}
};
Number result;

void *parse_INPUT(const SymbolContext<char>& ctx)
{
    result = ctx.value<Number>(1)->num;
}
void *parse_EXPR(const SymbolContext<char>& ctx)
{
    if (ctx.len() == 1)
        return ctx.value<Number>(1);
    else
        return new Number{ctx.value<Number>(1)->num + ctx.value<Number>(2)->num};
}
void *parse_TERM(const SymbolContext<char>& ctx)
{
    if (ctx.len() == 1)
        return ctx.value<Number>(1);
    else
        return new Number{ctx.value<Number>(1)->num * ctx.value<Number>(2)->num};
}
void *parse_FACT(const SymbolContext<char>& ctx)
{
    if (ctx.start()[0] == '(')
        return ctx.value<Number>(1);
    else
        return new Number(ctx.read());
}
```

#### Step 3. Generate and run the parser

```c++
int main(int argc, const char *argv[])
{
    Context<char> context{"/path/to/the/grammar.txt"};

    context.attach(L"INPUT", parse_INPUT);
    context.attach(L"EXPR", parse_EXPR);
    context.attach(L"TERM", parse_TERM);
    context.attach(L"FACT", parse_FACT);

    context.parse(argv[1], 1);

    printf("Result = %d\n", result);

    return 0;
}
```

## What is with the name?

Centaurus is a bright constellation low down in the southern sky (in Japan).
The alpha star of the constellation (*Alpha Centauri*, -0.1 mv) is also known as the closest star to Earth, 4.37 light years away.
The constellation was inspired from an imaginary animal which is a chimera between a human and a horse.

The name Centaurus of the software comes from the fact that it is a chimera between a parser and a lexer, like the imaginary animal.
The software is also seen as a chimera of PEG based methods (e.g. Packrat parsing) and LL(k) based methods,
being the first and only software (as of 2018) to generate a *scannerless* and *deterministic* parser.

The naming also follows the tradition in the field of parser development that a parser has to be named after a four-legged (and often domesticated) animal.
Yacc (LALR) is taken from yak, Bison (LALR/GLR) and Elkhound (GLR) from the animal with the same names and ANTLR (ALL(*)) from antlers (deer horns).