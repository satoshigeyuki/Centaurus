#pragma once

#include "atn.hpp"

namespace Centaurus
{
template<typename TCHAR> using LNFATransition = NFATransition<TCHAR>;

template<typename TCHAR> class LNFAState
{
};

template<typename TCHAR> class LNFA
{
    std::vector<LNFAState<TCHAR> > m_states;
private:
    void add_states(const NFA<TCHAR>& nfa, int origin)
    {
        const NFAState<TCHAR>& state = nfa.get_state(origin);

        for (const auto& tr : state.get_transitions())
        {
            if (tr.is_epsilon())
            {

            }
        }
    }
    void add_states(const ATN<TCHAR>& atn, int origin)
    {
        const ATNNode<TCHAR>& node = atn.get_node(origin);

        for (const auto& tr : node.get_transitions())
        {
        }
    }
public:
    LNFA(const ATN<TCHAR>& atn, int origin)
    {
    }
    virtual ~LNFA()
    {
    }
};
}
