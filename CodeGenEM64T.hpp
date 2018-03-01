#pragma once

#include "dfa.hpp"
#include "ldfa.hpp"
#include "asmjit/asmjit.h"

namespace Centaurus
{
template<typename TCHAR>
class ParserEM64T
{
    static constexpr int64_t AST_BUF_SIZE = 64 * 1024 * 1024;
    asmjit::JitRuntime m_runtime;
    asmjit::CodeHolder m_code;
    static CharClass<TCHAR> m_skipfilter;
    void emit_machine(asmjit::X86Compiler& cc, const ATNMachine<TCHAR>& machine, std::unordered_map<Identifier, asmjit::CCFunc*>& machine_map, const CompositeATN<TCHAR>& catn, const Identifier& id, asmjit::X86Mem astbuf_base, asmjit::X86Mem astbuf_head);
public:
    ParserEM64T(const Grammar<TCHAR>& grammar, asmjit::Logger *logger = NULL, asmjit::ErrorHandler *errhandler = NULL);
    virtual ~ParserEM64T() {}
    const void *operator()(const void *input, void *output)
    {
        const void *(*func)(const void *, void *);
        m_runtime.add(&func, &m_code);
        return func(input, output);
    }
};

template<typename TCHAR>
class DryParserEM64T
{
    asmjit::JitRuntime m_runtime;
    asmjit::CodeHolder m_code;
    static CharClass<TCHAR> m_skipfilter;
public:
    static void emit_machine(asmjit::X86Compiler& cc, const ATNMachine<TCHAR>& machine, std::unordered_map<Identifier, asmjit::CCFunc*>& machine_map, const CompositeATN<TCHAR>& catn, const Identifier& id);
    DryParserEM64T(const Grammar<TCHAR>& grammar, asmjit::Logger *logger = NULL);
    virtual ~DryParserEM64T() {}
    const void *operator()(const void *input)
    {
        const void *(*func)(const void *);
        m_runtime.add(&func, &m_code);
        return func(input);
    }
};

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