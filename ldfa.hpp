#pragma once

#include "catn.hpp"
#include "dfa.hpp"

namespace Centaurus
{
template<typename TCHAR>
class LookaheadDFA
{
public:
    LDFA(const CompositeATN<TCHAR>& catn, int origin)
    {
        std::set<int> ec0 = catn.epsilon_closure(origin);
    }
    virtual ~LDFA()
    {
    }
};
}
