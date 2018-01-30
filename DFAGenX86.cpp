#include "dfa.hpp"
#include "asmjit/asmjit.h"

namespace Centaurus
{
template<typename TCHAR>
class DFAFuncBuilderAMD64
{
	asmjit::X86Compiler m_cc;
	const DFA<TCHAR>& m_dfa;
	std::vector<asmjit::Label> m_labels;
private:
	void emit_state(int index)
	{
		m_cc.bind(m_labels[index]);

		m_cc.mov(asmjit::x86::al, asmjit::X86Mem(asmjit::x86::rcx, 0));
		m_cc.mov(asmjit::x86::ah, asmjit::x86::al);
		m_cc.inc(asmjit::x86::rcx);

		for (const auto& tr : m_dfa.get_transitions(index))
		{
			
			
			for (const auto& r : tr.label())
			{
				if (r.start() + 1 == r.end())
				{
					m_cc.cmp(asmjit::x86::al, r.start());
				}
				else
				{
					
				}
			}
		}
	}
public:
	DFAFuncBuilderAMD64(asmjit::CodeHolder& code, const DFA<TCHAR>& dfa)
		: m_cc(&code), m_dfa(dfa)
	{
		m_cc.addFunc(asmjit::FuncSignature1<void *, void *>(asmjit::CallConv::kIdHost));

		m_cc.setArg(0, asmjit::x86::rcx);

		for (int i = 0; i < dfa.get_state_num(); i++)
			labels.push_back(cc.newLabel());
	}
	virtual ~DFAFuncBuilderAMD64() {}
};

template<typename TCHAR>
class DFAFuncAMD64
{
	asmjit::CodeHolder code;
public:
	DFAFuncAMD64(const DFA<TCHAR>& dfa);
	virtual ~DFAFuncAMD64() {}
};

template<typename TCHAR>
DFAFuncAMD64<TCHAR>::DFAFuncAMD64(const DFA<TCHAR>& dfa)
{
}
}