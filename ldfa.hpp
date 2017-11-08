#pragma once

#include <vector>
#include <set>

#include "catn.hpp"
#include "dfa.hpp"
#include "nfa.hpp"

namespace Centaurus
{
template<typename TCHAR> using LDFATransition = NFATransition<TCHAR>;
template<typename TCHAR>
class LDFAState
{
    std::vector<LDFATransition<TCHAR> > m_states;
public:
    LDFAState()
    {
    }
    virtual ~LDFAState()
    {
    }
};
template<typename TCHAR>
class LookaheadDFA
{
    std::vector<LDFAState<TCHAR> > m_states;
    void fork_states(int origin, const CompositeATN<TCHAR>& atn)
    {
    }
public:
    LookaheadDFA(const CompositeATN<TCHAR>& catn, int origin)
    {
        std::set<int> ec0 = catn.epsilon_closure(origin);

        m_states.emplace_back(IndexVector(ec0.cbegin(), ec0.cend()));

        for (int node : ec0)
        {
            fork_states(0, catn);
        }
    }
    virtual ~LookaheadDFA()
    {
    }
};
}
