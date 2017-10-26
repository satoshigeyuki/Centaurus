#pragma once

#include "nfa.hpp"

namespace Centaur
{
template<typename TCHAR> using DFACharClass<TCHAR> = NFACharClass<TCHAR>;
template<typename TCHAR> using DFATransition<TCHAR> = NFATransition<TCHAR>;
template<typename TCHAR> using DFAState<TCHAR> = NFAState<TCHAR>;

template<typename TCHAR> class DFA
{
    std::vector<DFAState<TCHAR> > m_states;
public:
    DFA(const NFA<TCHAR>& nfa)
    {
    }
    virtual ~DFA()
    {
    }
};
}
