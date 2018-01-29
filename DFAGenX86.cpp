#include "dfa.hpp"
#include "asmjit/asmjit.h"

namespace Centaurus
{
template<typename TCHAR>
class DFAFuncAMD64
{
	asmjit::CodeHolder code;
public:
	DFAFuncAMD64(const DFA<TCHAR>& dfa);
	virtual ~DFAFuncAMD64() {}
};

template<typename TCHAR>
DFAFuncAMD64::DFAFuncAMD64(const DFA<TCHAR>& dfa)
{
	code.init(ArchInfo::kTypeX64);

	asmjit::X86Compiler cc(&code);

	cc.addFunc(asmjit::FuncSignature0<int>());



	cc.endFunc();
	cc.finalize();
}
}