#pragma once

#include <setjmp.h>
#include "dfa.hpp"
#include "ldfa.hpp"
#include "asmjit/asmjit.h"

namespace Centaurus
{
typedef const void *(*DFARoutine)(const void *, jmp_buf);
typedef int (*LDFARoutine)(const void *, jmp_buf);
typedef const void *(*MatchRoutine)(const void *, jmp_buf);
typedef const void *(*SkipRoutine)(const void *, jmp_buf);

class BaseParserEM64T
{
    asmjit::JitRuntime runtime;
    asmjit::CodeHolder code;
    asmjit::X86Compiler m_cc;
    asmjit::Label m_finishlabel;
    asmjit::X86Gp m_inputreg, m_jmpreg;
protected:
    void init()
    {
        m_cc.addFunc(asmjit::FuncSignature2<const void *, const void *, jmp_buf>(asmjit::CallConv::kIdHost));

        m_inputreg = m_cc.newIntPtr();
        m_cc.setArg(0, m_inputreg);

        m_jmpreg = m_cc.newIntPtr();
        m_cc.setArg(1, m_jmpreg);

        m_cc.spill(m_jmpreg);
    }
    void finalize()
    {
        m_cc.ret(m_inputreg);
        m_cc.finalize();
    }
public:
    BaseParserEM64T()
        : m_cc(&code)
    {
        code.init(runtime.getCodeInfo());
    }
    asmjit::X86Emitter& get_emitter()
    {
        return m_cc;
    }
    asmjit::X86Gp get_input_reg()
    {
        return m_inputreg;
    }
    asmjit::X86Gp get_jmpbuf_reg()
    {
        return m_jmpreg;
    }
    asmjit::X86Mem add_xmm_const(asmjit::Data128& data)
    {
        return m_cc.newXmmConst(asmjit::kConstScopeGlobal, data);
    }
    asmjit::X86Gp new_pointer_reg()
    {
        return m_cc.newIntPtr();
    }
    asmjit::X86Gp new_integer_reg()
    {
        return m_cc.newGpz();
    }
};

template<typename TCHAR>
class DFARoutineEM64T : public BaseParserEM64T
{
	asmjit::CodeHolder code;
public:
	DFARoutineEM64T(const asmjit::CodeInfo& codeinfo, const DFA<TCHAR>& dfa);
	virtual ~DFARoutineEM64T() {}
    DFARoutine getRoutine(asmjit::JitRuntime& runtime)
    {
        DFARoutine routine;
        runtime.add(&routine, &code);
        return routine;
    }
};
template<typename TCHAR>
class LDFARoutineEM64T : public BaseParserEM64T
{
	asmjit::CodeHolder code;
public:
    LDFARoutineEM64T(const asmjit::CodeInfo& codeinfo, const LookaheadDFA<TCHAR>& ldfa);
	virtual ~LDFARoutineEM64T() {}
    LDFARoutine getRoutine(asmjit::JitRuntime& runtime)
    {
        LDFARoutine routine;
        runtime.add(&routine, &code);
        return routine;
    }
};
template<typename TCHAR>
class MatchRoutineEM64T : public BaseParserEM64T
{
	asmjit::CodeHolder code;
public:
	MatchRoutineEM64T(const asmjit::CodeInfo& codeinfo, const std::basic_string<TCHAR>& str);
	virtual ~MatchRoutineEM64T() {}
    MatchRoutine getRoutine(asmjit::JitRuntime& runtime)
    {
        MatchRoutine routine;
        runtime.add(&routine, &code);
        return routine;
    }
};
template<typename TCHAR>
class SkipRoutineEM64T : public BaseParserEM64T
{
    asmjit::CodeHolder code;
public:
    SkipRoutineEM64T(const asmjit::CodeInfo& codeinfo, const CharClass<TCHAR>& cc);
    virtual ~SkipRoutineEM64T() {}
    SkipRoutine getRoutine(asmjit::JitRuntime& runtime)
    {
        SkipRoutine routine;
        runtime.add(&routine, &code);
        return routine;
    }
};
}