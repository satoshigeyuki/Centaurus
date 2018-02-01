#pragma once

#include "dfa.hpp"
#include "ldfa.hpp"
#include "asmjit/asmjit.h"

namespace Centaurus
{
typedef const void *(*DFARoutine)(const void *);
typedef int (*LDFARoutine)(const void *);
typedef const void *(*MatchRoutine)(const void *);

template<typename TCHAR>
class DFARoutineEM64T
{
	asmjit::CodeHolder code;
public:
	DFARoutineEM64T(const asmjit::CodeInfo& codeinfo, const DFA<TCHAR>& dfa);
	virtual ~DFARoutineEM64T() {}
    DFARoutine getRoutine(asmjit::JitRuntime& runtime)
    {
        DFARoutine routine;
        runtime.add(&routine, &code);
        return routine;
    }
};
template<typename TCHAR>
class LDFARoutineEM64T
{
	asmjit::CodeHolder code;
public:
    LDFARoutineEM64T(const asmjit::CodeInfo& codeinfo, const LookaheadDFA<TCHAR>& ldfa);
	virtual ~LDFARoutineEM64T() {}
    LDFARoutine getRoutine(asmjit::JitRuntime& runtime)
    {
        LDFARoutine routine;
        runtime.add(&routine, &code);
        return routine;
    }
};
template<typename TCHAR>
class MatchRoutineEM64T
{
	asmjit::CodeHolder code;
public:
	MatchRoutineEM64T(const asmjit::CodeInfo& codeinfo, const std::basic_string<TCHAR>& str);
	virtual ~MatchRoutineEM64T() {}
    MatchRoutine getRoutine(asmjit::JitRuntime& runtime)
    {
        MatchRoutine routine;
        runtime.add(&routine, &code);
        return routine;
    }
};
}