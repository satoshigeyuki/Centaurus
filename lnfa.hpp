#pragma once

#include <vector>
#include <unordered_map>
#include <functional>

#include "nfa.hpp"
#include "charclass.hpp"
#include "identifier.hpp"
#include "atn.hpp"

namespace Centaurus
{
template<typename TCHAR> using LNFATransition = NFATransition<TCHAR>;
template<typename TCHAR> using LNFAState = NFAState<TCHAR>;

template<typename TCHAR> class LNFA : public NFA<TCHAR>
{
    using NFA<TCHAR>::m_states;

    const std::unordered_map<Identifier, ATN<TCHAR> >& m_networks;
    std::vector<Identifier> m_stack;
    int m_maxdepth;
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
    int add_states_from_literal(const std::basic_string<TCHAR>& literal, int origin)
    {
        for (TCHAR ch : literal)
        {
            origin = add_state(CharClass<TCHAR>(ch, ch + 1), origin);
        }
        return origin;
    }
    void invoke_nonterminal(const Identifier& id, int origin)
    {
        if (m_stack.size() < m_maxdepth)
        {
            if (std::find(m_stack.cbegin(), m_stack.cend(), id) == m_stack.cend())
            {
                const ATN<TCHAR>& atn = m_networks.at(id);

                m_stack.push(id);

                import_state(atn, origin, 0);

                m_stack.pop(id);
            }
        }
    }
    void import_states(const ATN<TCHAR>& atn, int origin, int src)
    {
        const ATNNode<TCHAR>& node = atn.get_node(src);
        const std::vector<ATNTransition<TCHAR> >& transitions = node.get_transitions();

        for (unsigned int i = 0; i < transitions.size(); i++)
        {
            const ATNTransition<TCHAR>& tr = transitions[i];
            const ATNNode<TCHAR>& dest = atn.get_node(tr.dest());

            int next_origin;
            switch (dest.type())
            {
            case ATNNodeType::Blank:
                import_states(atn, origin, tr.dest());
                break;
            case ATNNodeType::LiteralTerminal:
                next_origin = add_states_from_literal(dest.get_literal(), origin);
                import_states(atn, next_origin, tr.dest());
                break;
            case ATNNodeType::RegularTerminal:
                next_origin = import_states(dest.get_nfa(), origin, 0);
                import_states(atn, next_origin, tr.dest());
                break;
            case ATNNodeType::Nonterminal:
                next_origin = invoke_nonterminal(dest.get_invoke());
                if (next_origin >= 0)
                    import_states(atn, next_origin, tr.dest());
                break;
            }
        }
    }
public:
    LNFA(const ATN<TCHAR>& atn, int root, const std::unordered_map<Identifier, ATN<TCHAR> >& networks, int maxdepth)
        : m_networks(networks), m_maxdepth(maxdepth)
    {
        m_states[0].set_label(0);
        import_states(atn, 0, root);
    }
    virtual ~LNFA()
    {
    }
};
}
