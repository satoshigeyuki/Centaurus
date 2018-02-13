#pragma once

#include "dfa.hpp"
#include "ldfa.hpp"
#include "asmjit/asmjit.h"

namespace Centaurus
{

/*
 * Base class for all parsing functions
 * Provides interface to the JIT (and AOT in the future) assembler
 */
/*class BaseParserEM64T
{
    asmjit::JitRuntime runtime;
    asmjit::CodeHolder code;
    asmjit::X86Compiler m_cc;
    asmjit::Label m_finishlabel;
    asmjit::X86Gp m_inputreg, m_jmpreg;
protected:
    void init()
    {
        m_cc.addFunc(asmjit::FuncSignature2<const void *, const void *, void *>(asmjit::CallConv::kIdHost));

        m_inputreg = m_cc.newIntPtr();
        m_cc.setArg(0, m_inputreg);

        m_jmpreg = m_cc.newIntPtr();
        m_cc.setArg(1, m_jmpreg);
    }
    void finalize()
    {
        m_cc.ret(m_inputreg);
        m_cc.endFunc();
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
};*/

template<typename TCHAR>
class DFARoutineEM64T
{
    asmjit::JitRuntime m_runtime;
    asmjit::CodeHolder m_code;
private:
    static void emit_state(asmjit::X86Compiler& cc, asmjit::X86Gp& inputreg, asmjit::X86Gp& backupreg, asmjit::Label& rejectlabel, const DFAState<TCHAR>& state, std::vector<asmjit::Label>& labels);
public:
    static void emit(asmjit::X86Compiler& cc, asmjit::X86Gp& inputreg, asmjit::Label& rejectlabel, const DFA<TCHAR>& dfa);
	DFARoutineEM64T(const DFA<TCHAR>& dfa, asmjit::Logger *logger = NULL);
	virtual ~DFARoutineEM64T() {}
    const void *operator()(const void *input)
    {
        const void *(*func)(const void *);
        m_runtime.add(&func, &m_code);
        return func(input);
    }
};

template<typename TCHAR>
class LDFARoutineEM64T
{
    asmjit::JitRuntime m_runtime;
    asmjit::CodeHolder m_code;
private:
    static void emit_state(asmjit::X86Compiler& cc, asmjit::X86Gp& inputreg, asmjit::Label& rejectlabel, const LDFAState<TCHAR>& state, std::vector<asmjit::Label>& labels, std::vector<asmjit::Label>& exitlabels);
public:
    static void emit(asmjit::X86Compiler& cc, asmjit::X86Gp& inputreg, asmjit::Label& rejectlabel, const LookaheadDFA<TCHAR>& ldfa, std::vector<asmjit::Label>& exitlabels);
    LDFARoutineEM64T(const LookaheadDFA<TCHAR>& ldfa, asmjit::Logger *logger = NULL);
	virtual ~LDFARoutineEM64T() {}
    int operator()(const void *input)
    {
        int (*func)(const void *);
        m_runtime.add(&func, &m_code);
        return func(input);
    }
};
template<typename TCHAR>
class MatchRoutineEM64T
{
    asmjit::JitRuntime m_runtime;
	asmjit::CodeHolder m_code;
public:
    static void emit(asmjit::X86Compiler& cc, asmjit::X86Gp& inputreg, asmjit::Label& rejectlabel, const std::basic_string<TCHAR>& str);
	MatchRoutineEM64T(const std::basic_string<TCHAR>& str, asmjit::Logger *logger = NULL);
	virtual ~MatchRoutineEM64T() {}
    const void *operator()(const void *input)
    {
        const void *(*func)(const void *);
        m_runtime.add(&func, &m_code);
        return func(input);
    }
};
template<typename TCHAR>
class SkipRoutineEM64T
{
    asmjit::JitRuntime m_runtime;
    asmjit::CodeHolder m_code;
private:
    static void emit_core(asmjit::X86Compiler& cc, asmjit::X86Gp& inputreg, asmjit::X86Xmm& filterreg);
public:
    static void emit(asmjit::X86Compiler& cc, asmjit::X86Gp& inputreg, const CharClass<TCHAR>& filter);
    SkipRoutineEM64T(const CharClass<TCHAR>& cc, asmjit::Logger *logger = NULL);
    virtual ~SkipRoutineEM64T() {}
    const void *operator()(const void *input)
    {
        const void *(*func)(const void *);
        m_runtime.add(&func, &m_code);
        return func(input);
    }
};
}