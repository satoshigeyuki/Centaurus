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
    }
    void add_states(const ATN<TCHAR>& atn, int origin)
    {
        const ATNNode<TCHAR>& node = atn.get_node(origin);


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
