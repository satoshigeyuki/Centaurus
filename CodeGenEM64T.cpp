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

#if 0
template<typename TCHAR>
class ParserEM64T : public BaseParserEM64T
{
    const Grammar<TCHAR>& m_grammar;
    std::unordered_map<Identifier, asmjit::Label> m_machinelabels;
public:
    ParserEM64T(const Grammar<TCHAR>& grammar)
        : m_grammar(grammar)
    {
        init();

        asmjit::X86Emitter& em = get_emitter();

        for (const auto& p : grammar.get_machines())
        {
            m_machinelabels.emplace(p.first, em.newLabel());
        }

        asmjit::Label finishlabel = em.newLabel();

        //The parsing starts by calling the root machine.
        //It returns only if the parsing reached EOF.
        em.call(m_machinelabels[grammar.get_root_id()]);
        //Jump to the epilog as we reached EOF.
        em.jmp(finishlabel);
        
        for (const auto& p : grammar.get_machines())
        {
            em.bind(m_machinelabels[p.first]);

            ATNMachineEM64T<TCHAR>::build(*this, p.second);
        }

        em.bind(finishlabel);

        finalize();
    }
    virtual ~ParserEM64T() {}
    const void *(*getRoutine)(const void *, jmp_buf)()
    {
        const void *(*routine)(const void *, jmp_buf);
        runtime.add(&routine, &code);
        return routine;
    }
    const void *run(const void *buf, jmp_buf jb)
    {
        const void *(*routine)(const void *, jmp_buf) = getRoutine();

        return routine(buf, jb);
    }
    asmjit::Label get_machine_label(const Identifier& id)
    {
        return m_machinelabels[id];
    }
};

template<typename TCHAR>
class ATNMachineEM64T
{
    ParserEM64T<TCHAR>& m_parser;
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
#endif

template<typename TCHAR>
DFARoutineEM64T<TCHAR>::DFARoutineEM64T(const DFA<TCHAR>& dfa, asmjit::Logger *logger)
{
    m_code.init(m_runtime.getCodeInfo());
    if (logger != NULL)
        m_code.setLogger(logger);

    asmjit::X86Compiler cc(&m_code);

    cc.addFunc(asmjit::FuncSignature1<const void *, const void *>(asmjit::CallConv::kIdHost));

    asmjit::X86Gp inputreg = cc.newIntPtr();
    cc.setArg(0, inputreg);
    asmjit::Label rejectlabel = cc.newLabel();

    emit(cc, inputreg, rejectlabel, dfa);

    cc.ret(inputreg);
    cc.endFunc();
    cc.finalize();
}

template<typename TCHAR>
void DFARoutineEM64T<TCHAR>::emit(asmjit::X86Compiler& cc, asmjit::X86Gp& inputreg, asmjit::Label& rejectlabel, const DFA<TCHAR>& dfa)
{
    std::vector<asmjit::Label> statelabels;

    for (int i = 0; i < dfa.get_state_num(); i++)
    {
        statelabels.push_back(cc.newLabel());
    }

    asmjit::Label finishlabel = cc.newLabel();
    asmjit::X86Gp backupreg = cc.newIntPtr();

    cc.mov(backupreg, 0);

    for (int i = 0; i < dfa.get_state_num(); i++)
    {
        cc.bind(statelabels[i]);

        emit_state(cc, inputreg, backupreg, finishlabel, dfa[i], statelabels);
    }
    cc.bind(finishlabel);
    cc.mov(inputreg, backupreg);
}

template<>
void DFARoutineEM64T<char>::emit_state(asmjit::X86Compiler& cc, asmjit::X86Gp& inputreg, asmjit::X86Gp& backupreg, asmjit::Label& rejectlabel, const DFAState<char>& state, std::vector<asmjit::Label>& labels)
{
    //The character is first read into a register from the stream
    asmjit::X86Gp cReg = cc.newGpz();
    //Another register is used for evacuating the character temporarily
	asmjit::X86Gp c2Reg = cc.newGpz();

    //Read the character from stream and advance the input position
	cc.movzx(cReg, asmjit::x86::byte_ptr(inputreg, 0));
	cc.mov(c2Reg, cReg);
	if (state.is_accept_state())
		cc.mov(backupreg, inputreg);
	cc.inc(inputreg);

	for (const auto& tr : state.get_transitions())
	{
		for (const auto& r : tr.label())
		{
			if (r.start() + 1 == r.end())
			{
                //The range consists of one character: test for equality and jump
				cc.cmp(cReg, r.start());
				cc.je(labels[tr.dest()]);
			}
			else
			{
                //The range consists of multiple characters: range check and jump
				cc.sub(cReg, r.start());
				cc.cmp(cReg, r.end() - r.start());
				cc.jb(labels[tr.dest()]);
				cc.mov(cReg, c2Reg);
			}
		}
	}
    //Jump to the "reject trampoline" and check if the input has ever been accepted
	cc.jmp(rejectlabel);
}

template<>
void DFARoutineEM64T<wchar_t>::emit_state(asmjit::X86Compiler& cc, asmjit::X86Gp& inputreg, asmjit::X86Gp& backupreg, asmjit::Label& rejectlabel, const DFAState<wchar_t>& state, std::vector<asmjit::Label>& labels)
{
    //The character is first read into a register from the stream
    asmjit::X86Gp cReg = cc.newGpz();
    //Another register is used for evacuating the character temporarily
    asmjit::X86Gp c2Reg = cc.newGpz();

    //Read the character from stream and advance the input position
    cc.movzx(cReg, asmjit::x86::word_ptr(inputreg, 0));
    cc.mov(c2Reg, cReg);
    if (state.is_accept_state())
        cc.mov(backupreg, inputreg);
    cc.add(inputreg, 2);

    for (const auto& tr : state.get_transitions())
    {
        for (const auto& r : tr.label())
        {
            if (r.start() + 1 == r.end())
            {
                //The range consists of one character: test for equality and jump
                cc.cmp(cReg, r.start());
                cc.je(labels[tr.dest()]);
            }
            else
            {
                //The range consists of multiple characters: range check and jump
                cc.sub(cReg, r.start());
                cc.cmp(cReg, r.end() - r.start());
                cc.jb(labels[tr.dest()]);
                cc.mov(cReg, c2Reg);
            }
        }
    }
    //Jump to the "reject trampoline" and check if the input has ever been accepted
    cc.jmp(rejectlabel);
}

template<typename TCHAR>
LDFARoutineEM64T<TCHAR>::LDFARoutineEM64T(const LookaheadDFA<TCHAR>& ldfa, asmjit::Logger *logger)
{
    m_code.init(m_runtime.getCodeInfo());
    if (logger != NULL)
        m_code.setLogger(logger);

    asmjit::X86Compiler cc(&m_code);

    cc.addFunc(asmjit::FuncSignature1<int, const void *>(asmjit::CallConv::kIdHost));

    asmjit::X86Gp inputreg = cc.newIntPtr();
    cc.setArg(0, inputreg);
    asmjit::Label rejectlabel = cc.newLabel();

    std::vector<asmjit::Label> exitlabels;

    int decision_num = ldfa.get_color_num();
    for (int i = 0; i < decision_num; i++)
    {
        exitlabels.push_back(cc.newLabel());
    }

    emit(cc, inputreg, rejectlabel, ldfa, exitlabels);
    cc.jmp(rejectlabel);

    for (int i = 0; i < decision_num; i++)
    {
        cc.bind(exitlabels[i]);
        cc.mov(inputreg, asmjit::Imm(i + 1));
        cc.ret(inputreg);
    }

    cc.bind(rejectlabel);
    cc.mov(inputreg, 0);
    cc.ret(inputreg);
    
    cc.endFunc();
    cc.finalize();
}

template<typename TCHAR>
void LDFARoutineEM64T<TCHAR>::emit(asmjit::X86Compiler& cc, asmjit::X86Gp& inputreg, asmjit::Label& rejectlabel, const LookaheadDFA<TCHAR>& ldfa, std::vector<asmjit::Label>& exitlabels)
{
    std::vector<asmjit::Label> statelabels;

    for (int i = 0; i < ldfa.get_state_num(); i++)
    {
        statelabels.push_back(cc.newLabel());
    }
    for (int i = 0; i < ldfa.get_state_num(); i++)
    {
        cc.bind(statelabels[i]);
        emit_state(cc, inputreg, rejectlabel, ldfa[i], statelabels, exitlabels);
    }
}

template<>
void LDFARoutineEM64T<char>::emit_state(asmjit::X86Compiler& cc, asmjit::X86Gp& inputreg, asmjit::Label& rejectlabel, const LDFAState<char>& state, std::vector<asmjit::Label>& labels, std::vector<asmjit::Label>& exitlabels)
{
    asmjit::X86Gp cReg = cc.newGpz();
    asmjit::X86Gp c2Reg = cc.newGpz();

    cc.movzx(cReg, asmjit::x86::byte_ptr(inputreg, 0));
    cc.mov(c2Reg, cReg);
    cc.inc(inputreg);

    for (const auto& tr : state.get_transitions())
    {
        for (const auto& r : tr.label())
        {
            if (r.start() + 1 == r.end())
            {
                cc.cmp(cReg, r.start());
                if (tr.dest() >= 0)
                {
                    cc.je(labels[tr.dest()]);
                }
                else
                {
                    cc.je(exitlabels[-tr.dest() - 1]);
                }
            }
            else
            {
                cc.sub(cReg, r.start());
                cc.cmp(cReg, r.end() - r.start());
                if (tr.dest() >= 0)
                {
                    cc.jb(labels[tr.dest()]);
                }
                else
                {
                    cc.jb(exitlabels[-tr.dest() - 1]);
                }
                cc.mov(cReg, c2Reg);
            }
        }
    }
    cc.jmp(rejectlabel);
}

template<>
void LDFARoutineEM64T<wchar_t>::emit_state(asmjit::X86Compiler& cc, asmjit::X86Gp& inputreg, asmjit::Label& rejectlabel, const LDFAState<wchar_t>& state, std::vector<asmjit::Label>& labels, std::vector<asmjit::Label>& exitlabels)
{
    //Nothing to note here. Just another specialization.

	asmjit::X86Gp cReg = cc.newGpz();
	asmjit::X86Gp c2Reg = cc.newGpz();

	cc.movzx(cReg, asmjit::x86::word_ptr(inputreg, 0));
	cc.mov(c2Reg, cReg);
	cc.add(inputreg, 2);

	for (const auto& tr : state.get_transitions())
	{
		for (const auto& r : tr.label())
		{
			if (r.start() + 1 == r.end())
			{
				cc.cmp(cReg, r.start());
				if (tr.dest() >= 0)
				{
					cc.je(labels[tr.dest()]);
				}
				else
				{
                    cc.je(exitlabels[-tr.dest() - 1]);
				}
			}
			else
			{
				cc.sub(cReg, r.start());
				cc.cmp(cReg, r.end() - r.start());
				if (tr.dest() >= 0)
				{
					cc.jb(labels[tr.dest()]);
				}
				else
				{
                    cc.jb(exitlabels[-tr.dest() - 1]);
				}
				cc.mov(cReg, c2Reg);
			}
		}
	}
    cc.jmp(rejectlabel);
}

template<typename TCHAR>
MatchRoutineEM64T<TCHAR>::MatchRoutineEM64T(const std::basic_string<TCHAR>& str, asmjit::Logger *logger)
{
    m_code.init(m_runtime.getCodeInfo());
    if (logger != NULL)
        m_code.setLogger(logger);

    asmjit::X86Compiler cc(&m_code);

    cc.addFunc(asmjit::FuncSignature2<const void *, const void *, void *>(asmjit::CallConv::kIdHost));

    asmjit::X86Gp inputreg = cc.newIntPtr();
    cc.setArg(0, inputreg);
    asmjit::Label rejectlabel = cc.newLabel();

    emit(cc, inputreg, rejectlabel, str);

    cc.ret(inputreg);

    cc.bind(rejectlabel);
    cc.mov(inputreg, 0);
    cc.ret(inputreg);
    
    cc.endFunc();
    cc.finalize();
}

template<>
void MatchRoutineEM64T<char>::emit(asmjit::X86Compiler& cc, asmjit::X86Gp& inputreg, asmjit::Label& rejectlabel, const std::basic_string<char>& str)
{
	asmjit::X86Gp mireg = cc.newIntPtr();
    asmjit::X86Xmm loadreg = cc.newXmm();

	for (int i = 0; i < str.size(); i += 16)
	{
		int l1 = std::min(str.size() - i, (size_t)16);

		asmjit::Data128 d1;

		for (int j = 0; j < l1; j++)
		{
			d1.ub[j] = str[i + j];
		}
		
		cc.vmovdqu(loadreg, asmjit::X86Mem(inputreg, 0));
		cc.vpcmpistri(loadreg, cc.newXmmConst(asmjit::kConstScopeGlobal, d1), asmjit::Imm(0x18), mireg);
		cc.cmp(mireg, asmjit::Imm(l1));
		cc.jb(rejectlabel);
		cc.add(inputreg, l1);
	}
}
template<>
void MatchRoutineEM64T<wchar_t>::emit(asmjit::X86Compiler& cc, asmjit::X86Gp& inputreg, asmjit::Label& rejectlabel, const std::basic_string<wchar_t>& str)
{
	asmjit::X86Gp mireg = cc.newIntPtr();
    asmjit::X86Xmm loadreg = cc.newXmm();

	for (int i = 0; i < str.size(); i += 8)
	{
		int l1 = std::min(str.size() - i, (size_t)8);

		asmjit::Data128 d1;

		for (int j = 0; j < l1; j++)
		{
			d1.uw[j] = str[i + j];
		}

		cc.vmovdqu(loadreg, asmjit::X86Mem(inputreg, 0));
		cc.vpcmpistri(loadreg, cc.newXmmConst(asmjit::kConstScopeGlobal, d1), asmjit::Imm(0x18), mireg);
		cc.cmp(mireg, asmjit::Imm(l1));
		cc.jb(rejectlabel);
		cc.add(inputreg, l1);
	}
}

template<typename TCHAR>
void SkipRoutineEM64T<TCHAR>::emit(asmjit::X86Compiler& cc, asmjit::X86Gp& inputreg, const CharClass<TCHAR>& filter)
{
    asmjit::X86Xmm filterreg = cc.newXmm();
    asmjit::X86Mem filtermem = cc.newXmmConst(asmjit::kConstScopeGlobal, pack_charclass(filter));
    
    cc.vmovdqa(filterreg, filtermem);

    asmjit::Label looplabel = cc.newLabel();

    cc.bind(looplabel);

    emit_core(cc, inputreg, filterreg);

    cc.jg(looplabel);
}

template<>
void SkipRoutineEM64T<char>::emit_core(asmjit::X86Compiler& cc, asmjit::X86Gp& inputreg, asmjit::X86Xmm& filterreg)
{
    asmjit::X86Xmm loadreg = cc.newXmm();
    asmjit::X86Gp mireg = cc.newGpz();

    cc.vmovdqu(loadreg, asmjit::X86Mem(inputreg, 0));
    cc.vpcmpistri(filterreg, loadreg, asmjit::Imm(0x14), mireg);
    cc.add(inputreg, mireg);
    cc.cmp(mireg, 15);
}

template<>
void SkipRoutineEM64T<wchar_t>::emit_core(asmjit::X86Compiler& cc, asmjit::X86Gp& inputreg, asmjit::X86Xmm& filterreg)
{
    asmjit::X86Xmm loadreg = cc.newXmm();
    asmjit::X86Gp mireg = cc.newGpz();

    cc.vmovdqu(loadreg, asmjit::X86Mem(inputreg, 0));
    cc.vpcmpistri(filterreg, loadreg, asmjit::Imm(0x15), mireg);
    cc.sal(mireg, 1);
    cc.add(inputreg, mireg);
    cc.cmp(mireg, 14);
}

template<typename TCHAR>
SkipRoutineEM64T<TCHAR>::SkipRoutineEM64T(const CharClass<TCHAR>& filter, asmjit::Logger *logger = NULL)
{
    m_code.init(m_runtime.getCodeInfo());
    if (logger != NULL)
        m_code.setLogger(logger);

    asmjit::X86Compiler cc(&m_code);

    cc.addFunc(asmjit::FuncSignature1<const void *, const void *>(asmjit::CallConv::kIdHost));

    asmjit::X86Gp inputreg = cc.newIntPtr();
    cc.setArg(0, inputreg);
    
    emit(cc, inputreg, filter);

    cc.ret(inputreg);
    cc.endFunc();
    cc.finalize();
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