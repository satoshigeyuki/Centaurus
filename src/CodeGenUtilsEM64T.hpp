#pragma once

#include "asmjit/asmjit.h"
#include "CharClass.hpp"
#include "CodeGenCommonEM64T.hpp"

namespace Centaurus
{
static asmjit::Data128 pack_charclass(const CharClass<char>& cc)
{
    asmjit::Data128 d128;

    for (int i = 0; i < 8; i++)
    {
        if (cc[i].empty())
        {
            d128.ub[i * 2 + 0] = 0xFF;
            d128.ub[i * 2 + 1] = 0;
        }
        else
        {
            d128.ub[i * 2 + 0] = cc[i].start();
            d128.ub[i * 2 + 1] = cc[i].end() - 1;
        }
    }

    return d128;
}

static asmjit::Data128 pack_charclass(const CharClass<wchar_t>& cc)
{
    asmjit::Data128 d128;

    for (int i = 0; i < 4; i++)
    {
        if (cc[i].empty())
        {
            d128.uw[i * 2 + 0] = 0xFFFF;
            d128.uw[i * 2 + 1] = 0;
        }
        else
        {
            d128.uw[i * 2 + 0] = cc[i].start();
            d128.uw[i * 2 + 1] = cc[i].end() - 1;
        }
    }

    return d128;
}

static void push_arg(asmjit::X86Assembler& as)
{
}

template<typename T1, typename... Ts>
static void push_arg(asmjit::X86Assembler& as, T1 arg1, Ts... args)
{
	push_arg(as, args...);
	as.push(arg1);
}

template<typename T, typename... Ts>
static void call_abs(asmjit::X86Assembler& as, T addr, Ts... args)
{
	push_arg(as, args...);
	if (sizeof...(args) >= 1) as.pop(ARG1_REG);
	if (sizeof...(args) >= 2) as.pop(ARG2_REG);
	if (sizeof...(args) >= 3) as.pop(ARG3_REG);
	if (sizeof...(args) >= 4) as.pop(ARG4_REG);
	
#if defined(CENTAURUS_BUILD_WINDOWS)
	as.sub(asmjit::x86::rsp, 32);
#endif
	as.call(reinterpret_cast<uint64_t>(addr));
#if defined(CENTAURUS_BUILD_WINDOWS)
	as.add(asmjit::x86::rsp, 32);
#endif
}

static void emit_parser_prolog(asmjit::X86Assembler& as)
{
    as.push(asmjit::x86::r9);
    as.push(asmjit::x86::r8);
    as.push(asmjit::x86::rbp);
    as.push(asmjit::x86::rdi);
    as.push(asmjit::x86::rsi);
    as.push(asmjit::x86::rdx);
    as.push(asmjit::x86::rcx);
    as.push(asmjit::x86::rbx);

    //Save RSP for bailout
    as.mov(asmjit::x86::r9, asmjit::x86::rsp);
}

static void emit_parser_epilog(asmjit::X86Assembler& as, asmjit::Label& rejectlabel)
{
    asmjit::Label acceptlabel = as.newLabel();

    as.mov(asmjit::x86::rax, INPUT_REG);
    as.jmp(acceptlabel);

    //Jump to this label is a long jump. Discard everything on the stack.
    as.bind(rejectlabel);
    as.mov(asmjit::x86::rax, 0);
    as.mov(asmjit::x86::rsp, asmjit::x86::r9);

    as.bind(acceptlabel);

    as.pop(asmjit::x86::rbx);
    as.pop(asmjit::x86::rcx);
    as.pop(asmjit::x86::rdx);
    as.pop(asmjit::x86::rsi);
    as.pop(asmjit::x86::rdi);
    as.pop(asmjit::x86::rbp);
    as.pop(asmjit::x86::r8);
    as.pop(asmjit::x86::r9);

    as.ret();
}

}
