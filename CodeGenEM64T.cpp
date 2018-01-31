#include "dfa.hpp"
#include "asmjit/asmjit.h"
#include "CodeGenEM64T.hpp"

namespace Centaurus
{
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
	void emit_reject_handler()
	{
		m_cc.bind(m_rejectlabel);

		m_cc.ret(m_backupReg);
	}
public:
	DFARoutineBuilderEM64T(asmjit::CodeHolder& code, const DFA<TCHAR>& dfa)
		: m_cc(&code), m_dfa(dfa)
	{
		m_cc.addFunc(asmjit::FuncSignature1<void *, void *>(asmjit::CallConv::kIdHost));

		m_inputReg = m_cc.newIntPtr();
		m_backupReg = m_cc.newIntPtr();

		//Set the initial position to RCX
		m_cc.setArg(0, m_input);
		//Set the backup position to NULL (no backup candidates)
		m_cc.mov(m_backupReg, 0);

		//Prepare one label for each state entry
		for (int i = 0; i < dfa.get_state_num(); i++)
			m_labels.push_back(cc.newLabel());

		//Labels for accept and reject states
		m_acceptlabel = cc.newLabel();
		m_rejectlabel = cc.newLabel();

		for (int i = 0; i < m_dfa.get_state_num(); i++)
		{
			emit_state(i);
		}
		emit_reject_handler();
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
class MatchRoutineBuilderEM64T
{
	asmjit::X86Compiler m_cc;
	const std::basic_string<TCHAR>& m_str;
private:
	void emit();
public:
	MatchRoutineBuilderEM64T(asmjit::CodeHolder& code, const std::basic_string<TCHAR>& str)
		: m_cc(&code), m_str(str)
	{
		m_cc.addFunc(asmjit::FuncSignature1<void *, void *>(asmjit::CallConv::kIdHost));

		emit();
	}
	virtual ~MatchRoutineBuilderEM64T() {}
};

template<> void MatchRoutineBuilderEM64T<char>::emit()
{
	
}
template<> void MatchRoutineBuilderEM64T<wchar_t>::emit()
{

}
}