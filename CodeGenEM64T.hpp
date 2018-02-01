#pragma once

#include "dfa.hpp"
#include "ldfa.hpp"
#include "asmjit/asmjit.h"

namespace Centaurus
{
template<typename TCHAR>
class DFARoutineEM64T
{
	asmjit::CodeHolder code;
public:
	void *(*run)(void *p);

	DFARoutineEM64T(const DFA<TCHAR>& dfa);
	virtual ~DFARoutineEM64T() {}
};
template<typename TCHAR>
class LDFARoutineEM64T
{
	asmjit::CodeHolder code;
public:
	int (*run)(void *p);

	LDFARoutineEM64T(const LookaheadDFA<TCHAR>& dfa);
	virtual ~LDFARoutineEM64T() {}
};
template<typename TCHAR>
class MatchRoutineEM64T
{
	asmjit::CodeHolder code;
public:
	void *(*run)(void *p);

	MatchRoutineEM64T(const std::basic_string<TCHAR>& str);
	virtual ~MatchRoutineEM64T() {}
};
}