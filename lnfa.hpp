#pragma once

#include <vector>
#include <unordered_map>

#include "nfa.hpp"
#include "charclass.hpp"
#include "identifier.hpp"
#include "atn.hpp"

namespace Centaurus
{
template<typename TCHAR> using LNFATransition = NFATransition<TCHAR>;

template<typename TCHAR> class LNFAState
{
    std::vector<LNFATransition<TCHAR> > m_transitions;
public:
    void add_transition(const CharClass<TCHAR>& cc, int dest)
    {
        m_transitions.emplace_back(cc, dest);
    }
};

template<typename TCHAR> class LNFABuilder
{
    const std::unordered_map<Identifier, ATN<TCHAR> >& m_networks;
    std::vector<Identifier> m_stack;
    int m_maxdepth;
public:
    LNFABuilder(const std::unordered_map<Identifier, ATN<TCHAR> >& networks, int maxdepth)
        : m_networks(networks), m_maxdepth(maxdepth)
    {
    }
    virtual ~LNFABuilder()
    {
    }
};

template<typename TCHAR> class LNFA
{
    std::vector<LNFAState<TCHAR> > m_states;
private:
    int add_state(const CharClass<TCHAR>& cc, int origin)
    {
        m_states[origin].add_transition(cc, m_states.size());
        m_states.emplace_back();
        return m_states.size() - 1;
    }
    void import_states(const NFA<TCHAR>& nfa, int origin, int src)
    {
        const NFAState<TCHAR>& state = nfa.get_state(src);

        for (const auto& tr : state.get_transitions())
        {
            if (tr.is_epsilon())
            {
                import_states(nfa, origin, tr.dest());
            }
            else
            {
                origin = add_state(tr.label());
                import_states(nfa, origin, tr.dest());
            }
        }
    }
    void add_states_from_literal(const std::basic_string<TCHAR>& literal, int origin)
    {
        for (TCHAR ch : literal)
        {
            add_state(tr.
        }
    }
    void import_states(LNFABuilder<TCHAR>& builder, const ATN<TCHAR>& atn, int origin, int src)
    {
        const ATNNode<TCHAR>& node = atn.get_node(src);

        for (const auto& tr : node.get_transitions())
        {
            const ATNNode<TCHAR>& dest = atn.get_node(tr.dest());

            switch (dest.type())
            {
            case ATNNodeType::Blank:
                import_states(atn, origin, tr.dest());
                break;
            case ATNNodeType::LiteralTerminal:
                break;
            case ATNNodeType::RegularTerminal:
                import_states(dest.get_nfa(), origin, 0);
                break;
            case ATNNodeType::Nonterminal:
                break;
            }
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
