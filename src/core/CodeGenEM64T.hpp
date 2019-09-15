#pragma once

#include "DFA.hpp"
#include "LookaheadDFA.hpp"
#include "asmjit/asmjit.h"
#include "BaseListener.hpp"
#include "CodeGenInterface.hpp"

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

        m_as.movdqa(dest, asmjit::X86Mem(m_label, offset));
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
class ChaserEM64T : public IChaser
{
	asmjit::JitRuntime m_runtime;
	asmjit::CodeHolder m_code;
	std::unordered_map<Identifier, ChaserFunc> m_funcmap;
    std::vector<ChaserFunc> m_funcarray;
	static CharClass<TCHAR> m_skipfilter;
	
	void emit_machine(asmjit::X86Assembler& as, const Grammar<TCHAR>& machine, const Identifier& id, const CompositeATN<TCHAR>& catn, asmjit::Label& rejectlabel, MyConstPool& pool);
	static void push_terminal(void *context, int id, const void *start, const void *end);
	static const void *request_nonterminal(void *context, int id, const void *input);
public:
    ChaserEM64T() {}
	ChaserEM64T(const Grammar<TCHAR>& grammar, asmjit::Logger *logger = NULL, asmjit::ErrorHandler *errhandler = NULL);
	void init(const Grammar<TCHAR>& grammar, asmjit::Logger *logger = NULL, asmjit::ErrorHandler *errhandler = NULL);
	virtual ~ChaserEM64T() {}
	ChaserFunc operator[](const Identifier& id) const
	{
		return m_funcmap.at(id);
	}
	ChaserFunc operator[](int id) const
	{
		return m_funcarray[id];
	}
};

template<typename TCHAR>
class ParserEM64T : public IParser
{
    static constexpr int64_t AST_BUF_SIZE = 8 * 1024 * 1024;
    asmjit::JitRuntime m_runtime;
    asmjit::CodeHolder m_code;
    static CharClass<TCHAR> m_skipfilter;
    const void *(*m_func)(void *context, const void *input, void *output);
    void emit_machine(asmjit::X86Assembler& as, const ATNMachine<TCHAR>& machine, std::unordered_map<Identifier, asmjit::Label>& machine_map, const CompositeATN<TCHAR>& catn, const Identifier& id, asmjit::Label& rejectlabel, MyConstPool& pool);
    static void *request_page(void *context);
public:
    ParserEM64T() {}
    ParserEM64T(const Grammar<TCHAR>& grammar, asmjit::Logger *logger = NULL, asmjit::ErrorHandler *errhandler = NULL);
    void init(const Grammar<TCHAR>& grammar, asmjit::Logger *logger = NULL, asmjit::ErrorHandler *errhandler = NULL);
    virtual ~ParserEM64T() {}
    const void *operator()(BaseListener *context, const void *input)
    {
        void *output = context->feed_callback();
        return m_func(context, input, output);
    }
};

template<typename TCHAR>
class DryParserEM64T : public IParser
{
    asmjit::JitRuntime m_runtime;
    asmjit::CodeHolder m_code;
    static CharClass<TCHAR> m_skipfilter;
public:
    static void emit_machine(asmjit::X86Assembler& as, const ATNMachine<TCHAR>& machine, std::unordered_map<Identifier, asmjit::Label>& machine_map, const CompositeATN<TCHAR>& catn, const Identifier& id, asmjit::Label& rejectlabel, MyConstPool& pool);
    DryParserEM64T(const Grammar<TCHAR>& grammar, asmjit::Logger *logger = NULL);
    virtual ~DryParserEM64T() {}
    const void *operator()(BaseListener *context, const void *input)
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
    static void emit_state(asmjit::X86Assembler& as, asmjit::Label& rejectlabel, const DFAState<TCHAR>& state, int index, std::vector<asmjit::Label>& labels, MyConstPool& pool);
public:
    static void emit(asmjit::X86Assembler& as, asmjit::Label& rejectlabel, const DFA<TCHAR>& dfa, MyConstPool& pool);
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
