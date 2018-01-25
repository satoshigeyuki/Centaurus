#pragma once

#include "nfa.hpp"

namespace Centaurus
{
template<typename TCHAR> using SDFATransition = NFATransition<TCHAR>;
template<typename TCHAR> using SDFAState = NFABaseState<TCHAR, IndexVector>;

template<typename TCHAR>
class SDFABasicBlock
{

};

template<typename TCHAR>
class StructuredDFA : public NFABase<SDFAState<TCHAR> >
{
public:
	StructuredDFA(const NFA<TCHAR>& nfa)
	{
		NFAClosure ec0 = nfa.epsilon_closure(0);


	}
	virtual ~StructuredDFA()
	{
	}
};
}