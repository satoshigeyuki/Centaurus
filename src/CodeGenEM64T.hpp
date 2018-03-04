#pragma once

#include "DFA.hpp"
#include "LookaheadDFA.hpp"
#include "asmjit/asmjit.h"

namespace Centaurus
{
class MyConstPool
{
    asmjit::X86Assembler& m_as;
    asmjit::Zone m_zone;
    asmjit::ConstPool m_pool;
    asmjit::Label m_label;
public:
    MyConstPool(asmjit::X86Assembler& as)
        : m_as(as), m_zone(1024), m_pool(&m_zone), m_label(as.newLabel())
    {
    }
    template<typename TCHAR>
    void load_charclass_filter(asmjit::X86Xmm dest, const CharClass<TCHAR>& cc)
    {
        asmjit::Data128 data = pack_charclass(cc);

        size_t offset;
        m_pool.add(&data, 16, offset);

        m_as.vmovdqa(dest, asmjit::X86Mem(m_label, offset));
    }
    void embed()
    {
        m_as.embedConstPool(m_label, m_pool);
    }
    asmjit::X86Mem add(asmjit::Data128 data)
    {
        size_t offset;
        m_pool.add(&data, 16, offset);

        return asmjit::X86Mem(m_label, offset);
    }
};
template<typename TCHAR>
class ParserEM64T
{
    static constexpr int64_t AST_BUF_SIZE = 8 * 1024 * 1024;
    asmjit::JitRuntime m_runtime;
    asmjit::CodeHolder m_code;
    static CharClass<TCHAR> m_skipfilter;
    void *m_buffer;
    const void *(*m_func)(void *context, const void *input, void *output);
    void emit_machine(asmjit::X86Assembler& as, const ATNMachine<TCHAR>& machine, std::unordered_map<Identifier, asmjit::Label>& machine_map, const CompositeATN<TCHAR>& catn, const Identifier& id, asmjit::Label& rejectlabel, MyConstPool& pool);
    static void * __cdecl request_page(void *context);
    long m_flipcount;
public:
    ParserEM64T(const Grammar<TCHAR>& grammar, asmjit::Logger *logger = NULL, asmjit::ErrorHandler *errhandler = NULL);
    virtual ~ParserEM64T() {}
    const void *operator()(const void *input)
    {
        m_flipcount = 0;
        return m_func(this, input, m_buffer);
    }
    void set_buffer(void *buffer)
    {
        m_buffer = buffer;
    }
    long get_flipcount() const
    {
        return m_flipcount;
    }
};

template<typename TCHAR>
class DryParserEM64T
{
    asmjit::JitRuntime m_runtime;
    asmjit::CodeHolder m_code;
    static CharClass<TCHAR> m_skipfilter;
public:
    static void emit_machine(asmjit::X86Assembler& as, const ATNMachine<TCHAR>& machine, std::unordered_map<Identifier, asmjit::Label>& machine_map, const CompositeATN<TCHAR>& catn, const Identifier& id, asmjit::Label& rejectlabel, MyConstPool& pool);
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
    static void emit_state(asmjit::X86Assembler& as, asmjit::Label& rejectlabel, const DFAState<TCHAR>& state, std::vector<asmjit::Label>& labels);
public:
    static void emit(asmjit::X86Assembler& as, asmjit::Label& rejectlabel, const DFA<TCHAR>& dfa);
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
    static void emit_state(asmjit::X86Assembler& as, asmjit::Label& rejectlabel, const LDFAState<TCHAR>& state, std::vector<asmjit::Label>& labels, std::vector<asmjit::Label>& exitlabels);
public:
    static void emit(asmjit::X86Assembler& as, asmjit::Label& rejectlabel, const LookaheadDFA<TCHAR>& ldfa, std::vector<asmjit::Label>& exitlabels);
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
    static void emit(asmjit::X86Assembler& as, MyConstPool& pool, asmjit::Label& rejectlabel, const std::basic_string<TCHAR>& str);
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
    static CharClass<TCHAR> m_skipfilter;
public:
    static void emit(asmjit::X86Assembler& as);
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