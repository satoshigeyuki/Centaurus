#pragma once

#include "nfa.hpp"

namespace Centaur
{
template<typename TCHAR> using DFACharClass = NFACharClass<TCHAR>;
template<typename TCHAR> using DFATransition = NFATransition<TCHAR>;
template<typename TCHAR> using DFAState = NFAState<TCHAR, std::vector<int> >;

template<typename TCHAR> class DFA
{
    std::vector<DFAState<TCHAR> > m_states;
public:
    DFA(const NFA<TCHAR>& nfa)
    {
        //Start by collecting the epsilon closure of start state
        std::vector<int> ec0 = nfa.epsilon_closure(0);

        //States reached via epsilon transitions from ec0
        std::vector<NFATransition<TCHAR>&> et0 = nfa.gather_transitions(ec0);
    }
    virtual ~DFA()
    {
    }
};
}
