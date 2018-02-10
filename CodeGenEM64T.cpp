#include "dfa.hpp"
#include "asmjit/asmjit.h"
#include "CodeGenEM64T.hpp"

namespace Centaurus
{
asmjit::Data128 pack_charclass(const CharClass<char>& cc)
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
asmjit::Data128 pack_charclass(const CharClass<wchar_t>& cc)
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

template<typename TCHAR>
class BaseParserEM64T
{
    asmjit::JitRuntime runtime;
    asmjit::CodeHolder code;
    asmjit::X86Assembler m_as;
    asmjit::ConstPool *m_pool;
    asmjit::FuncFrameLayout m_layout;
    asmjit::Label m_escapelabel, m_finishlabel;
    asmjit::Label m_stackbackup, m_poollabel;
protected:
    void init()
    {
        asmjit::FuncDetail func;
        func.init(asmjit::FuncSignature1<const void *, const void *>(asmjit::CallConv::kIdHost));

        asmjit::FuncFrameInfo ffi;
        ffi.setDirtyRegs(asmjit::X86Reg::kKindVec, asmjit::Utils::mask(0, 1));

        asmjit::X86Gp inputreg = get_input_reg();

        asmjit::FuncArgsMapper mapper(&func);
        mapper.assignAll(inputreg);
        mapper.updateFrameInfo(ffi);

        m_layout.init(func, ffi);

        asmjit::FuncUtils::emitProlog(&m_as, m_layout);
        asmjit::FuncUtils::allocArgs(&m_as, m_layout, mapper);

        //A constant pool is shared among all the machines
        m_pool = m_as.newConstPool();

        m_escapelabel = m_as.newLabel();
        m_finishlabel = m_as.newLabel();

        m_stackbackup = m_as.newLabel();
        m_as.mov(asmjit::X86Mem(m_stackbackup, 0), m_as.zsp());

        m_poollabel = m_as.newLabel();
    }
    void finalize()
    {
        //This place is reached when one of the "reject trampolines" is stepped upon.
        m_as.bind(m_escapelabel);
        //Rewind the stack to the saved position.
        m_as.mov(m_as.zsp(), asmjit::X86Mem(m_stackbackup, 0));
        m_as.mov(get_input_reg(), 0);

        m_as.bind(m_finishlabel);
        m_as.mov(m_as.zax(), get_input_reg());
        asmjit::FuncUtils::emitEpilog(&m_as, m_layout);

        m_as.align(asmjit::kAlignZero, 8);
        m_as.bind(m_stackbackup);
        m_as.embed(NULL, 8);

        m_as.embedConstPool()
    }
public:
    BaseParserEM64T()
        : runtime(), code(runtime.getCodeInfo()), m_as(&code)
    {
        
    }
    asmjit::X86Emitter& get_emitter()
    {
        return m_as;
    }
    asmjit::Label& get_escape_label()
    {
        return m_escapelabel;
    }
    asmjit::Label& get_finish_label()
    {
        return m_finishlabel;
    }
    asmjit::ConstPool *get_const_pool()
    {
        return m_pool;
    }
    asmjit::X86Gp get_input_reg()
    {
        //Input position is tracked by RCX/ECX throughout the parser
        return m_as.zcx();
    }
    asmjit::X86Mem add_xmm_const(asmjit::Data128& data)
    {
        size_t offset;
        m_pool->add(data.ub, 16, offset);
        return 
    }
};

template<typename TCHAR>
class ParserEM64T : public BaseParserEM64T<TCHAR>
{
    const Grammar<TCHAR>& m_grammar;
    std::unordered_map<Identifier, asmjit::Label> m_machinelabels;
public:
    ParserEM64T(const Grammar<TCHAR>& grammar)
        : m_grammar(grammar)
    {
        for (const auto& p : grammar.get_machines())
        {
            m_machinelabels.emplace(p.first, m_as.newLabel());
        }

        asmjit::Label finishlabel = m_as.newLabel();
        asmjit::Label stackbackup = m_as.newLabel();

        //The parsing starts by calling the root machine.
        //It returns only if the parsing reached EOF.
        m_as.call(m_machinelabels[grammar.get_root_id()]);
        //Jump to the epilog as we reached EOF.
        m_as.jmp(finishlabel);

        m_as.bind(finishlabel);
        m_as.mov(m_as.zax(), inputreg);

        

        for (const auto& p : grammar.get_machines())
        {
            m_machinelabels.emplace(p.first, m_as.newLabel());
        }
        for (const auto& p : grammar.get_machines())
        {
            m_as.bind(m_machinelabels[p.first]);

            emit_machine(p.second);
        }
    }
    virtual ~ParserEM64T() {}
    const void *(*getRoutine)(const void *)()
    {
        const void *(*routine)(const void *);
        runtime.add(&routine, &code);
        return routine;
    }
    const void *run(const void *buf)
    {
        const void *(*routine)(const void *) = getRoutine();

        return routine(buf);
    }
};

template<typename TCHAR>
class ATNMachineEM64T
{
    asmjit::X86Emitter m_as;
    std::vector<asmjit::Label> m_statelabels;
    const ATNMachine<TCHAR>& m_machine;
private:
    void emit_state(int index)
    {
        const ATNNode<TCHAR>& state = m_machine.get_node(index);

        switch (state.type())
        {
        case ATNNodeType::Blank:
            //Do nothing. Just continue to the next node within the flow.
            break;
        case ATNNodeType::LiteralTerminal:
            MatchRoutineBuilderEM64T<TCHAR>::build(m_as, *m_pool, state.get_literal());
            break;
        case ATNNodeType::Nonterminal:
            m_as.call(m_machinelabels[state.get_invoke()]);
            break;
        case ATNNodeType::RegularTerminal:
            DFARoutineBuilderEM64T<TCHAR>::build(m_as, state.get_nfa());
            break;
        case ATNNodeType::WhiteSpace:
            SkipRoutineBuilderEM64T<TCHAR>::build(m_as, m_whitespacelabel);
            break;
        }

        if (state.get_transitions().size() == 1)
        {
            m_as.jmp(m_statelabels[state.get_transitions(0).dest()]);
        }
        else if (state.get_transitions().empty())
        {
            assert(index == m_machine.get_node_num() - 1);
        }
        else
        {
            LDFARoutineBuilderEM64T<TCHAR>::build()
        }
    }
public:
    ATNMachineEM64T(asmjit::X86Emitter& emitter, const ATNMachine<TCHAR>& machine)
        : m_as(emitter), m_machine(machine)
    {
        for (int i = 0; i < machine.get_node_num(); i++)
        {
            m_statelabels.push_back(m_as.newLabel());
        }

        for (int i = 0; i < machine.get_node_num(); i++)
        {
            emit_state(i);
        }
        m_as.ret();
    }
    virtual ~ATNMachineEM64T() {}
    static void build(asmjit::X86Emitter& emitter, const ATNMachine<TCHAR>& machine)
    {
        ATNMachineEM64T<TCHAR> builder(emitter, machine);
    }
};

template<typename TCHAR>
class DFARoutineBuilderEM64T
{
    ParserEM64T<TCHAR>& m_parser;
	const DFA<TCHAR>& m_dfa;
	std::vector<asmjit::Label> m_labels;
	asmjit::Label m_rejectlabel;
	asmjit::X86Gp m_inputReg, m_backupReg;
private:
	void emit_state(int index);
public:
    DFARoutineBuilderEM64T(ParserEM64T<TCHAR>& parser, const DFA<TCHAR>& dfa)
		: m_parser(parser), m_dfa(dfa)
	{
        asmjit::X86Emitter& em = m_parser.get_emitter();

        //Input cursor assigned to RCX/ECX
        m_inputReg = em.zcx();
        //Backup register, which stores the last accepted position, assigned to RDX/EDX
        m_backupReg = em.zdx();

		//Set the backup position to NULL (no backup candidates)
		em.mov(m_backupReg, 0);

		//Prepare one label for each state entry
		for (int i = 0; i < m_dfa.get_state_num(); i++)
			m_labels.push_back(em.newLabel());

		//Label for reject state
		m_rejectlabel = em.newLabel();

		for (int i = 0; i < m_dfa.get_state_num(); i++)
		{
            em.bind(m_labels[index]);

			emit_state(i);
		}
		
		em.bind(m_rejectlabel);

        //The backup register contains 0 if the accept states are never reached
        em.cmp(m_backupReg, 0);

        //Jump to the escape label when the input is rejected
        //(Reject trampoline)
        em.jz(m_parser.get_escape_label());

        //Proceed to the next node within the machine
	}
	virtual ~DFARoutineBuilderEM64T() {}
};

template<> void DFARoutineBuilderEM64T<char>::emit_state(int index)
{
    asmjit::X86Emitter& em = m_parser.get_emitter();

    //The character is first read into RAX/EAX from the stream
	asmjit::X86Gp cReg = em.zax();
    //RBX/EBX is used for evacuating the character temporarily
	asmjit::X86Gp c2Reg = em.zbx();

    //Read the character from stream and advance the input position
	em.movzx(cReg, asmjit::x86::byte_ptr(m_inputReg, 0));
	em.mov(c2Reg, cReg);
	if (m_dfa.is_accept_state(index))
		em.mov(m_backupReg, m_inputReg);
	em.inc(m_inputReg);

	for (const auto& tr : m_dfa.get_transitions(index))
	{
		for (const auto& r : tr.label())
		{
			if (r.start() + 1 == r.end())
			{
                //The range consists of one character: test for equality and jump
				em.cmp(cReg, r.start());
				em.je(m_labels[tr.dest()]);
			}
			else
			{
                //The range consists of multiple characters: range check and jump
				em.sub(cReg, r.start());
				em.cmp(cReg, r.end() - r.start());
				em.jb(m_labels[tr.dest()]);
				em.mov(cReg, c2Reg);
			}
		}
	}
    //Jump to the "reject trampoline" and check if the input has ever been accepted
	em.jmp(m_rejectlabel);
}

template<> void DFARoutineBuilderEM64T<wchar_t>::emit_state(int index)
{
    //The above function overloaded for wide characters
    asmjit::X86Emitter& em = m_parser.get_emitter();

	asmjit::X86Gp cReg = em.zax();
	asmjit::X86Gp c2Reg = em.zbx();

    //The source pointer is designated to be word wide
	em.movzx(cReg, asmjit::x86::word_ptr(m_inputReg, 0));
	em.mov(c2Reg, cReg);
    //The input position advances by two bytes
	if (m_dfa.is_accept_state(index))
		em.mov(m_backupReg, m_inputReg);
	em.add(m_inputReg, 2);

    //There is nothing different below
	for (const auto& tr : m_dfa.get_transitions(index))
	{
		for (const auto& r : tr.label())
		{
			if (r.start() + 1 == r.end())
			{
				em.cmp(cReg, r.start());
				em.je(m_labels[tr.dest()]);
			}
			else
			{
				em.sub(cReg, r.start());
				em.cmp(cReg, r.end() - r.start());
				em.jb(m_labels[tr.dest()]);
				em.mov(cReg, c2Reg);
			}
		}
	}
	em.jmp(m_rejectlabel);
}

template<typename TCHAR>
class LDFARoutineBuilderEM64T
{
    ParserEM64T<TCHAR>& m_parser;
	const LookaheadDFA<TCHAR>& m_ldfa;
	asmjit::X86Gp m_inputReg;
	std::vector<asmjit::Label> m_exitlabels;
	std::vector<asmjit::Label> m_labels;
private:
	void emit_state(int index);
public:
	LDFARoutineBuilderEM64T(ParserEM64T<TCHAR>& parser, const LookaheadDFA<TCHAR>& ldfa)
		: m_parser(parser), m_ldfa(ldfa)
	{
        asmjit::X86Emitter& em = m_parser.get_emitter();

        //The input pointer assigned to RCX/ECX
        m_inputReg = em.zcx();

        //Allocate one label for each decision
        int decision_num = m_ldfa.get_color_num();
        for (int i = 0; i < decision_num; i++)
        {
            m_exitlabels.push_back(em.newLabel());
        }

        //Allocate one label for each state
		for (int i = 0; i < m_ldfa.get_state_num(); i++)
		{
			m_labels.push_back(em.newLabel());
		}

		for (int i = 0; i < m_ldfa.get_state_num(); i++)
		{
            em.bind(m_labels[index]);

			emit_state(i);
		}
	}
	virtual ~LDFARoutineBuilderEM64T() {}
};

template<> void LDFARoutineBuilderEM64T<char>::emit_state(int index)
{
    asmjit::X86Emitter& em = m_parser.get_emitter();

    //The character is first read into RAX/EAX
	asmjit::X86Gp cReg = em.zax();
    //RBX/EBX is used for evacuating the character temporarily
	asmjit::X86Gp c2Reg = em.zbx();

    //Read the character from stream and advance the pointer
    //We do not need to remember the accept states
	em.movzx(cReg, asmjit::x86::byte_ptr(m_inputReg, 0));
	em.mov(c2Reg, cReg);
	em.inc(m_inputReg);

	for (const auto& tr : m_ldfa.get_transitions(index))
	{
		for (const auto& r : tr.label())
		{
			if (r.start() + 1 == r.end())
			{
                //The range consists of just one character
				em.cmp(cReg, r.start());
				if (tr.dest() >= 0)
				{
                    //Transition to the next state within LDFA
					em.je(m_labels[tr.dest()]);
				}
				else
				{
                    //Lookahead is finished. We jump to some unbound label
                    em.je(m_exitlabels[-tr.dest() - 1]);
				}
			}
			else
			{
                //The range consists of multiple characters
				em.sub(cReg, r.start());
				em.cmp(cReg, r.end() - r.start());
				if (tr.dest() >= 0)
				{
					em.jb(m_labels[tr.dest()]);
				}
				else
				{
                    em.jb(m_exitlabels[-tr.dest() - 1]);
				}
                //Restore the evacuated character
				em.mov(cReg, c2Reg);
			}
		}
	}
    //The input sequence may be rejected even during lookahead. (Early reject)
    //We need the "reject trampoline" in the LDFA routine too.
    em.jmp(m_parser.get_escape_label());
}

template<> void LDFARoutineBuilderEM64T<wchar_t>::emit_state(int index)
{
    //Nothing to note here. Just another specialization.

    asmjit::X86Emitter& em = m_parser.get_emitter();

	asmjit::X86Gp cReg = em.zax();
	asmjit::X86Gp c2Reg = em.zbx();

	em.movzx(cReg, asmjit::x86::word_ptr(m_inputReg, 0));
	em.mov(c2Reg, cReg);
	em.add(m_inputReg, 2);

	for (const auto& tr : m_ldfa.get_transitions(index))
	{
		for (const auto& r : tr.label())
		{
			if (r.start() + 1 == r.end())
			{
				em.cmp(cReg, r.start());
				if (tr.dest() >= 0)
				{
					em.je(m_labels[tr.dest()]);
				}
				else
				{
                    em.je(m_exitlabels[-tr.dest() - 1]);
				}
			}
			else
			{
				em.sub(cReg, r.start());
				em.cmp(cReg, r.end() - r.start());
				if (tr.dest() >= 0)
				{
					em.jb(m_labels[tr.dest()]);
				}
				else
				{
                    em.jb(m_exitlabels[-tr.dest() - 1]);
				}
				em.mov(cReg, c2Reg);
			}
		}
	}
    em.jmp(m_parser.get_escape_label());
}

template<typename TCHAR>
class MatchRoutineBuilderEM64T
{
    ParserEM64T<TCHAR>& m_parser;
	const std::basic_string<TCHAR>& m_str;
	asmjit::X86Gp m_inputReg;
    asmjit::Label m_rejectlabel;
private:
	void emit();
public:
	MatchRoutineBuilderEM64T(ParserEM64T<TCHAR>& parser, const std::basic_string<TCHAR>& str)
		: m_parser(parser), m_str(str)
	{
        asmjit::X86Emitter& em = m_parser.get_emitter();

        m_inputReg = em.zcx();
        m_rejectlabel = em.newLabel();

		emit();

        //Reject trampoline.
        em.bind(m_rejectlabel);
        em.jmp(m_parser.get_escape_label());
	}
	virtual ~MatchRoutineBuilderEM64T() {}
};

template<> void MatchRoutineBuilderEM64T<char>::emit()
{
    asmjit::X86Emitter& em = m_parser.get_emitter();

	asmjit::X86Gp miReg = em.zcx();
    asmjit::X86Xmm loadReg = asmjit::x86::xmm(0);

	for (int i = 0; i < m_str.size(); i += 16)
	{
		int l1 = std::min(m_str.size() - i, (size_t)16);

		asmjit::Data128 d1;

		for (int j = 0; j < l1; j++)
		{
			d1.ub[j] = m_str[i + j];
		}
		
		em.vmovdqu(loadReg, asmjit::X86Mem(m_inputReg, 0));
		em.vpcmpistri(loadReg, patMem, asmjit::Imm(0x18), miReg);
		em.cmp(miReg, asmjit::Imm(l1));
		em.jb(m_rejectlabel);
		em.add(m_inputReg, l1);
	}
}
template<> void MatchRoutineBuilderEM64T<wchar_t>::emit()
{
    asmjit::X86Emitter& em = m_parser.get_emitter();

	asmjit::X86Gp miReg = em.zcx();
    asmjit::X86Xmm loadReg = asmjit::x86::xmm(0);

	for (int i = 0; i < m_str.size(); i += 8)
	{
		int l1 = std::min(m_str.size() - i, (size_t)8);

		asmjit::Data128 d1;

		for (int j = 0; j < l1; j++)
		{
			d1.uw[j] = m_str[i + j];
		}

		em.vmovdqu(loadReg, asmjit::X86Mem(m_inputReg, 0));
		em.vpcmpistri(loadReg, patMem, asmjit::Imm(0x18), miReg);
		em.cmp(miReg, asmjit::Imm(l1));
		em.jb(m_rejectlabel);
		em.add(m_inputReg, l1);
	}
}

template<typename TCHAR>
class SkipRoutineBuilderEM64T
{
    ParserEM64T<TCHAR>& m_parser;
    CharClass<TCHAR> m_filter;
    asmjit::X86Gp m_inputReg;
    asmjit::X86Xmm m_filterReg;
private:
    void emit();
public:
    SkipRoutineBuilderEM64T(ParserEM64T<TCHAR>& parser, const CharClass<TCHAR>& cc)
        : m_parser(parser), m_filter(cc)
    {
        asmjit::X86Emitter& em = m_parser.get_emitter();

        m_inputReg = em.zcx();

        m_filterReg = asmjit::x86::xmm(0);

        asmjit::X86Mem filterMem = m_cc.newXmmConst(asmjit::kConstScopeLocal, pack_charclass(m_filter));
        em.vmovdqa(m_filterReg, filterMem);

        asmjit::Label looplabel = em.newLabel();

        em.bind(looplabel);

        emit();

        em.jg(looplabel);
    }
    virtual ~SkipRoutineBuilderEM64T() {}
};

template<> void SkipRoutineBuilderEM64T<char>::emit()
{
    asmjit::X86Emitter& em = m_parser.get_emitter();

    asmjit::X86Xmm loadReg = asmjit::x86::xmm(1);
    asmjit::X86Gp miReg = em.zcx();

    em.vmovdqu(loadReg, asmjit::X86Mem(m_inputReg, 0));
    em.vpcmpistri(m_filterReg, loadReg, asmjit::Imm(0x14), miReg);
    em.add(m_inputReg, miReg);
    em.cmp(miReg, 15);
}

template<> void SkipRoutineBuilderEM64T<wchar_t>::emit()
{
    asmjit::X86Emitter& em = m_parser.get_emitter();

    asmjit::X86Xmm loadReg = asmjit::x86::xmm(1);
    asmjit::X86Gp miReg = em.zcx();

    em.vmovdqu(loadReg, asmjit::X86Mem(m_inputReg, 0));
    em.vpcmpistri(m_filterReg, loadReg, asmjit::Imm(0x15), miReg);
    em.sal(miReg, 1);
    em.add(m_inputReg, miReg);
    em.cmp(miReg, 14);
}

template<typename TCHAR>
DFARoutineEM64T<TCHAR>::DFARoutineEM64T(const asmjit::CodeInfo& codeinfo, const DFA<TCHAR>& dfa)
{
    code.init(codeinfo);

    asmjit::X86Compiler compiler(&code);

    compiler.addFunc(asmjit::FuncSignature1<const void *, const void *>());

	DFARoutineBuilderEM64T<TCHAR> builder(code, dfa);

    compiler.endFunc();
    compiler.finalize();
}

template<typename TCHAR>
LDFARoutineEM64T<TCHAR>::LDFARoutineEM64T(const asmjit::CodeInfo& codeinfo, const LookaheadDFA<TCHAR>& ldfa)
{
    code.init(codeinfo);

    asmjit::X86Compiler compiler(&code);

    compiler.addFunc(asmjit::FuncSignature1<int, const void *>());

	LDFARoutineBuilderEM64T<TCHAR> builder(code, ldfa);

    compiler.endFunc();
    compiler.finalize();
}

template<typename TCHAR>
MatchRoutineEM64T<TCHAR>::MatchRoutineEM64T(const asmjit::CodeInfo& codeinfo, const std::basic_string<TCHAR>& str)
{
    code.init(codeinfo);

    asmjit::X86Compiler compiler(&code);

    compiler.addFunc(asmjit::FuncSignature1<const void *, const void *>());

	MatchRoutineBuilderEM64T<TCHAR> builder(code, str);

    compiler.endFunc();
    compiler.finalize();
}

template<typename TCHAR>
SkipRoutineEM64T<TCHAR>::SkipRoutineEM64T(const asmjit::CodeInfo& codeinfo, const CharClass<TCHAR>& cc)
{
    code.init(codeinfo);

    asmjit::X86Compiler compiler(&code);

    compiler.addFunc(asmjit::FuncSignature1<const void *, const void *>());

    SkipRoutineBuilderEM64T<TCHAR> builder(code, cc);

    compiler.endFunc();
    compiler.finalize();
}

template class DFARoutineEM64T<char>;
template class DFARoutineEM64T<wchar_t>;
template class LDFARoutineEM64T<char>;
template class LDFARoutineEM64T<wchar_t>;
template class MatchRoutineEM64T<char>;
template class MatchRoutineEM64T<wchar_t>;
template class SkipRoutineEM64T<char>;
template class SkipRoutineEM64T<wchar_t>;
}