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
class DFARoutineBuilderEM64T
{
	asmjit::X86Compiler m_cc;
	const DFA<TCHAR>& m_dfa;
	std::vector<asmjit::Label> m_labels;
	asmjit::Label m_rejectlabel, m_acceptlabel;
	asmjit::X86Gp m_inputReg, m_backupReg;
private:
	void emit_state(int index);
public:
	DFARoutineBuilderEM64T(asmjit::CodeHolder& code, const DFA<TCHAR>& dfa)
		: m_cc(&code), m_dfa(dfa)
	{
		m_cc.addFunc(asmjit::FuncSignature1<const void *, const void *>(/*asmjit::CallConv::kIdHost*/));

		m_inputReg = m_cc.newIntPtr();
		m_backupReg = m_cc.newIntPtr();

		//Set the initial position to RCX
		m_cc.setArg(0, m_inputReg);
		//Set the backup position to NULL (no backup candidates)
		m_cc.mov(m_backupReg, 0);

		//Prepare one label for each state entry
		for (int i = 0; i < m_dfa.get_state_num(); i++)
			m_labels.push_back(m_cc.newLabel());

		//Labels for accept and reject states
		m_acceptlabel = m_cc.newLabel();
		m_rejectlabel = m_cc.newLabel();

		for (int i = 0; i < m_dfa.get_state_num(); i++)
		{
			emit_state(i);
		}
		
		m_cc.bind(m_rejectlabel);
		m_cc.ret(m_backupReg);

		m_cc.endFunc();
		m_cc.finalize();
	}
	virtual ~DFARoutineBuilderEM64T() {}
};

template<> void DFARoutineBuilderEM64T<char>::emit_state(int index)
{
	m_cc.bind(m_labels[index]);

	asmjit::X86Gp cReg = m_cc.newGpd();
	asmjit::X86Gp c2Reg = m_cc.newGpd();

	m_cc.movzx(cReg, asmjit::x86::byte_ptr(m_inputReg, 0));
	m_cc.mov(c2Reg, cReg);
	if (m_dfa.is_accept_state(index))
		m_cc.mov(m_backupReg, m_inputReg);
	m_cc.inc(m_inputReg);

	for (const auto& tr : m_dfa.get_transitions(index))
	{
		for (const auto& r : tr.label())
		{
			if (r.start() + 1 == r.end())
			{
				m_cc.cmp(cReg, r.start());
				m_cc.je(m_labels[tr.dest()]);
			}
			else
			{
				m_cc.sub(cReg, r.start());
				m_cc.cmp(cReg, r.end() - r.start());
				m_cc.jb(m_labels[tr.dest()]);
				m_cc.mov(cReg, c2Reg);
			}
		}
	}
	m_cc.jmp(m_rejectlabel);
}

template<> void DFARoutineBuilderEM64T<wchar_t>::emit_state(int index)
{
	m_cc.bind(m_labels[index]);

	asmjit::X86Gp cReg = m_cc.newGpd();
	asmjit::X86Gp c2Reg = m_cc.newGpd();

	m_cc.movzx(cReg, asmjit::x86::word_ptr(m_inputReg, 0));
	m_cc.mov(c2Reg, cReg);

	if (m_dfa.is_accept_state(index))
		m_cc.mov(m_backupReg, m_inputReg);
	m_cc.add(m_inputReg, 2);

	for (const auto& tr : m_dfa.get_transitions(index))
	{
		for (const auto& r : tr.label())
		{
			if (r.start() + 1 == r.end())
			{
				m_cc.cmp(cReg, r.start());
				m_cc.je(m_labels[tr.dest()]);
			}
			else
			{
				m_cc.sub(cReg, r.start());
				m_cc.cmp(cReg, r.end() - r.start());
				m_cc.jb(m_labels[tr.dest()]);
				m_cc.mov(cReg, c2Reg);
			}
		}
	}
	m_cc.jmp(m_rejectlabel);
}

template<typename TCHAR>
class LDFARoutineBuilderEM64T
{
	asmjit::X86Compiler m_cc;
	const LookaheadDFA<TCHAR>& m_ldfa;
	asmjit::X86Gp m_inputReg, m_resultReg;
	asmjit::Label m_exitlabel;
	std::vector<asmjit::Label> m_labels;
private:
	void emit_state(int index);
public:
	LDFARoutineBuilderEM64T(asmjit::CodeHolder& code, const LookaheadDFA<TCHAR>& ldfa)
		: m_cc(&code), m_ldfa(ldfa)
	{
		m_cc.addFunc(asmjit::FuncSignature1<int, const void *>(asmjit::CallConv::kIdHost));

		m_inputReg = m_cc.newIntPtr();
		m_cc.setArg(0, m_inputReg);

		m_resultReg = m_cc.newGpz();

		m_exitlabel = m_cc.newLabel();

		for (int i = 0; i < m_ldfa.get_state_num(); i++)
		{
			m_labels.push_back(m_cc.newLabel());
		}

		m_cc.mov(m_resultReg, -1);

		for (int i = 0; i < m_ldfa.get_state_num(); i++)
		{
			emit_state(i);
		}

		m_cc.bind(m_exitlabel);
		
		m_cc.ret(m_resultReg);

		m_cc.endFunc();
		m_cc.finalize();
	}
	virtual ~LDFARoutineBuilderEM64T() {}
};

template<> void LDFARoutineBuilderEM64T<char>::emit_state(int index)
{
	m_cc.bind(m_labels[index]);

	asmjit::X86Gp cReg = m_cc.newGpd();
	asmjit::X86Gp c2Reg = m_cc.newGpd();

	m_cc.movzx(cReg, asmjit::x86::byte_ptr(m_inputReg, 0));
	m_cc.mov(c2Reg, cReg);
	m_cc.inc(m_inputReg);

	for (const auto& tr : m_ldfa.get_transitions(index))
	{
		for (const auto& r : tr.label())
		{
			if (r.start() + 1 == r.end())
			{
				m_cc.cmp(cReg, r.start());
				if (tr.dest() >= 0)
				{
					m_cc.je(m_labels[tr.dest()]);
				}
				else
				{
					m_cc.mov(m_resultReg, asmjit::Imm(-tr.dest()));
					m_cc.je(m_exitlabel);
				}
			}
			else
			{
				m_cc.sub(cReg, r.start());
				m_cc.cmp(cReg, r.end() - r.start());
				if (tr.dest() >= 0)
				{
					m_cc.jb(m_labels[tr.dest()]);
				}
				else
				{
					m_cc.mov(m_resultReg, asmjit::Imm(-tr.dest()));
					m_cc.jb(m_exitlabel);
				}
				m_cc.mov(cReg, c2Reg);
			}
		}
	}
	m_cc.jmp(m_exitlabel);
}

template<> void LDFARoutineBuilderEM64T<wchar_t>::emit_state(int index)
{
	m_cc.bind(m_labels[index]);

	asmjit::X86Gp cReg = m_cc.newGpd();
	asmjit::X86Gp c2Reg = m_cc.newGpd();

	m_cc.movzx(cReg, asmjit::x86::word_ptr(m_inputReg, 0));
	m_cc.mov(c2Reg, cReg);
	m_cc.add(m_inputReg, 2);

	for (const auto& tr : m_ldfa.get_transitions(index))
	{
		for (const auto& r : tr.label())
		{
			if (r.start() + 1 == r.end())
			{
				m_cc.cmp(cReg, r.start());
				if (tr.dest() >= 0)
				{
					m_cc.je(m_labels[tr.dest()]);
				}
				else
				{
					m_cc.mov(m_resultReg, asmjit::Imm(-tr.dest()));
					m_cc.je(m_exitlabel);
				}
			}
			else
			{
				m_cc.sub(cReg, r.start());
				m_cc.cmp(cReg, r.end() - r.start());
				if (tr.dest() >= 0)
				{
					m_cc.jb(m_labels[tr.dest()]);
				}
				else
				{
					m_cc.mov(m_resultReg, asmjit::Imm(-tr.dest()));
					m_cc.jb(m_exitlabel);
				}
				m_cc.mov(cReg, c2Reg);
			}
		}
	}
	m_cc.jmp(m_exitlabel);
}

template<typename TCHAR>
class MatchRoutineBuilderEM64T
{
	asmjit::X86Compiler m_cc;
	const std::basic_string<TCHAR>& m_str;
	asmjit::X86Gp m_inputReg;
	asmjit::Label m_rejectlabel;
private:
	void emit();
public:
	MatchRoutineBuilderEM64T(asmjit::CodeHolder& code, const std::basic_string<TCHAR>& str)
		: m_cc(&code), m_str(str)
	{
		m_cc.addFunc(asmjit::FuncSignature1<const void *, const void *>(asmjit::CallConv::kIdHost));

		m_rejectlabel = m_cc.newLabel();

        m_inputReg = m_cc.newIntPtr();
		m_cc.setArg(0, m_inputReg);

		emit();

		m_cc.bind(m_rejectlabel);

		m_cc.mov(m_inputReg, 0);
		m_cc.ret(m_inputReg);

		m_cc.endFunc();
		m_cc.finalize();
	}
	virtual ~MatchRoutineBuilderEM64T() {}
};

template<> void MatchRoutineBuilderEM64T<char>::emit()
{
	asmjit::X86Gp miReg = m_cc.newGpd();
	asmjit::X86Xmm loadReg = m_cc.newXmm();

	for (int i = 0; i < m_str.size(); i += 16)
	{
		int l1 = std::min(m_str.size() - i, (size_t)16);

		asmjit::Data128 d1;

		for (int j = 0; j < l1; j++)
		{
			d1.ub[j] = m_str[i + j];
		}

		asmjit::X86Mem patMem = m_cc.newXmmConst(asmjit::kConstScopeLocal, d1);
		
		m_cc.vmovdqu(loadReg, asmjit::X86Mem(m_inputReg, 0));
		m_cc.vpcmpistri(loadReg, patMem, asmjit::Imm(0x18), miReg);
		m_cc.cmp(miReg, asmjit::Imm(l1));
		m_cc.jb(m_rejectlabel);
		m_cc.add(m_inputReg, l1);
	}

	m_cc.ret(m_inputReg);
}
template<> void MatchRoutineBuilderEM64T<wchar_t>::emit()
{
	asmjit::X86Gp miReg = m_cc.newGpd();
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

		m_cc.vmovdqu(loadReg, asmjit::X86Mem(m_inputReg, 0));
		m_cc.vpcmpistri(loadReg, patMem, asmjit::Imm(0x18), miReg);
		m_cc.cmp(miReg, asmjit::Imm(l1));
		m_cc.jb(m_rejectlabel);
		m_cc.add(m_inputReg, l1);
	}

	m_cc.ret(m_inputReg);
}

template<typename TCHAR>
class SkipRoutineBuilderEM64T
{
    asmjit::X86Compiler m_cc;
    CharClass<TCHAR> m_filter;
    asmjit::X86Gp m_inputReg;
    asmjit::X86Xmm m_filterReg;
private:
    void emit();
public:
    SkipRoutineBuilderEM64T(asmjit::CodeHolder& code, const CharClass<TCHAR>& cc)
        : m_cc(&code), m_filter(cc)
    {
        m_cc.addFunc(asmjit::FuncSignature1<const void *, const void *>(asmjit::CallConv::kIdHost));

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

        m_cc.endFunc();
        m_cc.finalize();
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

	DFARoutineBuilderEM64T<TCHAR> builder(code, dfa);
}

template<typename TCHAR>
LDFARoutineEM64T<TCHAR>::LDFARoutineEM64T(const asmjit::CodeInfo& codeinfo, const LookaheadDFA<TCHAR>& ldfa)
{
    code.init(codeinfo);

	LDFARoutineBuilderEM64T<TCHAR> builder(code, ldfa);
}

template<typename TCHAR>
MatchRoutineEM64T<TCHAR>::MatchRoutineEM64T(const asmjit::CodeInfo& codeinfo, const std::basic_string<TCHAR>& str)
{
    code.init(codeinfo);

	MatchRoutineBuilderEM64T<TCHAR> builder(code, str);
}

template<typename TCHAR>
SkipRoutineEM64T<TCHAR>::SkipRoutineEM64T(const asmjit::CodeInfo& codeinfo, const CharClass<TCHAR>& cc)
{
    code.init(codeinfo);

    SkipRoutineBuilderEM64T<TCHAR> builder(code, cc);
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