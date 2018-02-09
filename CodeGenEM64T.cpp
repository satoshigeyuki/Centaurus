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
class ParserEM64T
{
    asmjit::X86Builder m_as;
    const Grammar<TCHAR>& m_grammar;
    std::unordered_map<Identifier, asmjit::Label> m_machinelabels;
    asmjit::Label m_bailoutlabel;
private:
    void emit_machine(const ATNMachine<TCHAR>& machine)
    {
        for (const auto& state : machine)
        {
        }
    }
public:
    ParserEM64T(asmjit::CodeHolder& code, const Grammar<TCHAR>& grammar)
        : m_as(&code), m_grammar(grammar)
    {
        asmjit::FuncDetail func;
        func.init(asmjit::FuncSignature1<const void *, const void *>(asmjit::CallConv::kIdHost));

        asmjit::FuncFrameInfo ffi;
        ffi.setDirtyRegs(asmjit::X86Reg::kKindVec, asmjit::Utils::mask(0, 1));

        asmjit::X86Gp inputreg = m_as.zcx();

        asmjit::FuncArgsMapper mapper(&func);
        mapper.assignAll(inputreg);
        mapper.updateFrameInfo(ffi);

        asmjit::FuncFrameLayout layout;
        layout.init(func, ffi);

        asmjit::FuncUtils::emitProlog(&m_as, layout);
        asmjit::FuncUtils::allocArgs(&m_as, layout, mapper);

        for (const auto& p : grammar.get_machines())
        {
            m_machinelabels.emplace(p.first, m_as.newLabel());
        }
        for (const auto& p : grammar.get_machines())
        {
            m_as.bind(m_machinelabels[p.first]);

            emit_machine(p.second);
        }

        asmjit::FuncUtils::emitEpilog(&m_as, layout);
    }
    virtual ~ParserEM64T() {}
};

template<typename TCHAR>
class DFARoutineBuilderEM64T
{
    asmjit::X86Emitter& m_as;
	const DFA<TCHAR>& m_dfa;
	std::vector<asmjit::Label> m_labels;
	asmjit::Label m_rejectlabel, m_escapelabel;
	asmjit::X86Gp m_inputReg, m_backupReg;
private:
	void emit_state(int index);
public:
    DFARoutineBuilderEM64T(asmjit::X86Emitter &emitter, asmjit::Label escapeLabel, const DFA<TCHAR>& dfa)
		: m_as(emitter), m_escapeLabel(escapeLabel), m_dfa(dfa)
	{
        m_inputReg = m_as.zcx();
        m_backupReg = m_as.zdx();

		//Set the backup position to NULL (no backup candidates)
		m_as.mov(m_backupReg, 0);

		//Prepare one label for each state entry
		for (int i = 0; i < m_dfa.get_state_num(); i++)
			m_labels.push_back(m_as.newLabel());

		//Label for reject state
		m_rejectlabel = m_as.newLabel();

		for (int i = 0; i < m_dfa.get_state_num(); i++)
		{
			emit_state(i);
		}
		
		m_as.bind(m_rejectlabel);
        m_as.cmp(m_backupReg, 0);

        //Jump to the escape label when the input is rejected
        m_as.jz(escapeLabel);

        //Proceed to the next node within the machine
	}
	virtual ~DFARoutineBuilderEM64T() {}
};

template<> void DFARoutineBuilderEM64T<char>::emit_state(int index)
{
	m_as.bind(m_labels[index]);

	asmjit::X86Gp cReg = m_as.zax();
	asmjit::X86Gp c2Reg = m_as.zbx();

	m_as.movzx(cReg, asmjit::x86::byte_ptr(m_inputReg, 0));
	m_as.mov(c2Reg, cReg);
	if (m_dfa.is_accept_state(index))
		m_as.mov(m_backupReg, m_inputReg);
	m_as.inc(m_inputReg);

	for (const auto& tr : m_dfa.get_transitions(index))
	{
		for (const auto& r : tr.label())
		{
			if (r.start() + 1 == r.end())
			{
				m_as.cmp(cReg, r.start());
				m_as.je(m_labels[tr.dest()]);
			}
			else
			{
				m_as.sub(cReg, r.start());
				m_as.cmp(cReg, r.end() - r.start());
				m_as.jb(m_labels[tr.dest()]);
				m_as.mov(cReg, c2Reg);
			}
		}
	}
	m_as.jmp(m_rejectlabel);
}

template<> void DFARoutineBuilderEM64T<wchar_t>::emit_state(int index)
{
	m_as.bind(m_labels[index]);

	asmjit::X86Gp cReg = m_as.zax();
	asmjit::X86Gp c2Reg = m_as.zax();

	m_as.movzx(cReg, asmjit::x86::word_ptr(m_inputReg, 0));
	m_as.mov(c2Reg, cReg);

	if (m_dfa.is_accept_state(index))
		m_as.mov(m_backupReg, m_inputReg);
	m_as.add(m_inputReg, 2);

	for (const auto& tr : m_dfa.get_transitions(index))
	{
		for (const auto& r : tr.label())
		{
			if (r.start() + 1 == r.end())
			{
				m_as.cmp(cReg, r.start());
				m_as.je(m_labels[tr.dest()]);
			}
			else
			{
				m_as.sub(cReg, r.start());
				m_as.cmp(cReg, r.end() - r.start());
				m_as.jb(m_labels[tr.dest()]);
				m_as.mov(cReg, c2Reg);
			}
		}
	}
	m_as.jmp(m_rejectlabel);
}

template<typename TCHAR>
class LDFARoutineBuilderEM64T
{
    asmjit::X86Emitter& m_as;
	const LookaheadDFA<TCHAR>& m_ldfa;
	asmjit::X86Gp m_inputReg;
	std::vector<asmjit::Label> m_exitlabels;
	std::vector<asmjit::Label> m_labels;
    asmjit::Label m_escapeLabel;
private:
	void emit_state(int index);
public:
	LDFARoutineBuilderEM64T(asmjit::X86Emitter& emitter, asmjit::Label escapeLabel, const LookaheadDFA<TCHAR>& ldfa)
		: m_as(emitter), m_escapeLabel(escapeLabel), m_ldfa(ldfa)
	{
        m_inputReg = m_as.zcx();

        int decision_num = m_ldfa.get_color_num();
        for (int i = 0; i < decision_num; i++)
        {
            m_exitlabels.push_back(m_as.newLabel());
        }

		for (int i = 0; i < m_ldfa.get_state_num(); i++)
		{
			m_labels.push_back(m_as.newLabel());
		}

		for (int i = 0; i < m_ldfa.get_state_num(); i++)
		{
			emit_state(i);
		}
	}
	virtual ~LDFARoutineBuilderEM64T() {}
};

template<> void LDFARoutineBuilderEM64T<char>::emit_state(int index)
{
	m_as.bind(m_labels[index]);

	asmjit::X86Gp cReg = m_as.zax();
	asmjit::X86Gp c2Reg = m_as.zbx();

	m_as.movzx(cReg, asmjit::x86::byte_ptr(m_inputReg, 0));
	m_as.mov(c2Reg, cReg);
	m_as.inc(m_inputReg);

	for (const auto& tr : m_ldfa.get_transitions(index))
	{
		for (const auto& r : tr.label())
		{
			if (r.start() + 1 == r.end())
			{
				m_as.cmp(cReg, r.start());
				if (tr.dest() >= 0)
				{
					m_as.je(m_labels[tr.dest()]);
				}
				else
				{
                    m_as.je(m_exitlabels[-tr.dest() - 1]);
				}
			}
			else
			{
				m_as.sub(cReg, r.start());
				m_as.cmp(cReg, r.end() - r.start());
				if (tr.dest() >= 0)
				{
					m_as.jb(m_labels[tr.dest()]);
				}
				else
				{
                    m_as.jb(m_exitlabels[-tr.dest() - 1]);
				}
				m_as.mov(cReg, c2Reg);
			}
		}
	}
    m_as.jmp(m_escapeLabel);
}

template<> void LDFARoutineBuilderEM64T<wchar_t>::emit_state(int index)
{
	m_as.bind(m_labels[index]);

	asmjit::X86Gp cReg = m_as.zax();
	asmjit::X86Gp c2Reg = m_as.zbx();

	m_as.movzx(cReg, asmjit::x86::word_ptr(m_inputReg, 0));
	m_as.mov(c2Reg, cReg);
	m_as.add(m_inputReg, 2);

	for (const auto& tr : m_ldfa.get_transitions(index))
	{
		for (const auto& r : tr.label())
		{
			if (r.start() + 1 == r.end())
			{
				m_as.cmp(cReg, r.start());
				if (tr.dest() >= 0)
				{
					m_as.je(m_labels[tr.dest()]);
				}
				else
				{
                    m_as.je(m_exitlabels[-tr.dest() - 1]);
				}
			}
			else
			{
				m_as.sub(cReg, r.start());
				m_as.cmp(cReg, r.end() - r.start());
				if (tr.dest() >= 0)
				{
					m_as.jb(m_labels[tr.dest()]);
				}
				else
				{
                    m_as.jb(m_exitlabels[-tr.dest() - 1]);
				}
				m_as.mov(cReg, c2Reg);
			}
		}
	}
    m_as.jmp(m_escapeLabel);
}

template<typename TCHAR>
class MatchRoutineBuilderEM64T
{
    asmjit::X86Emitter& m_as;
	const std::basic_string<TCHAR>& m_str;
	asmjit::X86Gp m_inputReg;
	asmjit::Label m_escapeLabel;
private:
	void emit();
public:
	MatchRoutineBuilderEM64T(asmjit::X86Emitter& emitter, asmjit::Label escapeLabel, const std::basic_string<TCHAR>& str)
		: m_as(emitter), m_escapeLabel(escapeLabel), m_str(str)
	{
        m_inputReg = m_as.zcx();

		emit();
	}
	virtual ~MatchRoutineBuilderEM64T() {}
};

template<> void MatchRoutineBuilderEM64T<char>::emit()
{
	asmjit::X86Gp miReg = m_as.zcx();
    asmjit::X86Xmm loadReg = asmjit::x86::xmm(0);

	for (int i = 0; i < m_str.size(); i += 16)
	{
		int l1 = std::min(m_str.size() - i, (size_t)16);

		asmjit::Data128 d1;

		for (int j = 0; j < l1; j++)
		{
			d1.ub[j] = m_str[i + j];
		}

		asmjit::X86Mem patMem = m_as.newXmmConst(asmjit::kConstScopeLocal, d1);
		
		m_as.vmovdqu(loadReg, asmjit::X86Mem(m_inputReg, 0));
		m_as.vpcmpistri(loadReg, patMem, asmjit::Imm(0x18), miReg);
		m_as.cmp(miReg, asmjit::Imm(l1));
		m_as.jb(m_rejectlabel);
		m_as.add(m_inputReg, l1);
	}
}
template<> void MatchRoutineBuilderEM64T<wchar_t>::emit()
{
	asmjit::X86Gp miReg = m_as.zcx();
	asmjit::X86Xmm loadReg = m_cc.newXmm();

	for (int i = 0; i < m_str.size(); i += 8)
	{
		int l1 = std::min(m_str.size() - i, (size_t)8);

		asmjit::Data128 d1;

		for (int j = 0; j < l1; j++)
		{
			d1.uw[j] = m_str[i + j];
		}

		asmjit::X86Mem patMem = m_cc.newXmmConst(asmjit::kConstScopeLocal, d1);

		m_as.vmovdqu(loadReg, asmjit::X86Mem(m_inputReg, 0));
		m_as.vpcmpistri(loadReg, patMem, asmjit::Imm(0x18), miReg);
		m_as.cmp(miReg, asmjit::Imm(l1));
		m_as.jb(m_rejectlabel);
		m_as.add(m_inputReg, l1);
	}
}

template<typename TCHAR>
class SkipRoutineBuilderEM64T
{
    CharClass<TCHAR> m_filter;
    asmjit::X86Gp m_inputReg;
    asmjit::X86Xmm m_filterReg;
private:
    void emit();
public:
    SkipRoutineBuilderEM64T(asmjit::CodeHolder& code, const CharClass<TCHAR>& cc)
        : m_cc(&code), m_filter(cc)
    {
        m_inputReg = m_cc.newIntPtr();
        m_cc.setArg(0, m_inputReg);
        m_filterReg = m_cc.newXmm();

        asmjit::X86Mem filterMem = m_cc.newXmmConst(asmjit::kConstScopeLocal, pack_charclass(m_filter));
        m_cc.vmovdqa(m_filterReg, filterMem);

        asmjit::Label looplabel = m_cc.newLabel();

        m_cc.bind(looplabel);

        emit();

        m_cc.jg(looplabel);
        m_cc.ret(m_inputReg);
    }
    virtual ~SkipRoutineBuilderEM64T() {}
};

template<> void SkipRoutineBuilderEM64T<char>::emit()
{
    asmjit::X86Xmm loadReg = m_cc.newXmm();
    asmjit::X86Gp miReg = m_cc.newGpz();

    m_cc.vmovdqu(loadReg, asmjit::X86Mem(m_inputReg, 0));
    m_cc.vpcmpistri(m_filterReg, loadReg, asmjit::Imm(0x14), miReg);
    m_cc.add(m_inputReg, miReg);
    m_cc.cmp(miReg, 15);
}

template<> void SkipRoutineBuilderEM64T<wchar_t>::emit()
{
    asmjit::X86Xmm loadReg = m_cc.newXmm();
    asmjit::X86Gp miReg = m_cc.newGpz();

    m_cc.vmovdqu(loadReg, asmjit::X86Mem(m_inputReg, 0));
    m_cc.vpcmpistri(m_filterReg, loadReg, asmjit::Imm(0x15), miReg);
    m_cc.sal(miReg, 1);
    m_cc.add(m_inputReg, miReg);
    m_cc.cmp(miReg, 14);
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