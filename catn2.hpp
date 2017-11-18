#pragma once

#include <unordered_map>
#include <vector>
#include "identifier.hpp"
#include "nfa.hpp"
#include "atn.hpp"

namespace Centaurus
{
template<typename TCHAR> using CATNTransition = NFATransition<TCHAR>;
template<typename TCHAR>
class CATNNode
{
    std::vector<CATNTransition<TCHAR> > m_transitions;
    Identifier m_submachine;
public:
    CATNNode()
    {
    }
    CATNNode(const Identifier& id)
        : m_submachine(id)
    {
    }
    virtual ~CATNNode()
    {
    }
    void add_transition(const CharClass<TCHAR>& cc, int dest)
    {
        m_transitions.emplace_back(cc, dest);
    }
    void import_transitions(const NFAState<TCHAR>& state, int offset)
    {
        for (const auto& tr : state.get_transitions())
        {
            m_transitions.push_back(tr.offset(offset));
        }
    }
    const std::vector<CATNTransition<TCHAR> >& get_transitions() const
    {
        return m_transitions;
    }
    void print(std::ostream& os, int from) const
    {
        for (const auto& t : m_transitions)
        {
            os << "N" << from << " -> N" << t.dest() << " [ label=\"";
            os << t.label();
            os << "\" ];" << std::endl;
        }
    }
};

template<typename TCHAR>
class CompositeATN
{
    std::vector<CATNNode<TCHAR> > m_nodes;
private:
    int add_node(const CharClass<TCHAR>& cc, int origin)
    {
        if (!m_nodes.empty() && origin >= 0)
            m_nodes[origin].add_transition(cc, m_nodes.size());
        m_nodes.emplace_back();
        return m_nodes.size() - 1;
    }
    int add_node(const CATNNode<TCHAR>& node, int origin)
    {
        if (!m_nodes.empty() && origin >= 0)
            m_nodes[origin].add_transition(CharClass<TCHAR>(), m_nodes.size());
        m_nodes.push_back(node);
        return m_nodes.size() - 1;
    }
    int import_literal_terminal(const std::basic_string<TCHAR>& literal, int origin)
    {
        for (TCHAR ch : literal)
        {
            origin = add_node(CharClass<TCHAR>(ch, ch + 1), origin);
        }
        return origin;
    }
    int import_regular_terminal(const NFA<TCHAR>& nfa, int origin)
    {
        int offset = m_nodes.size();
        add_node(CharClass<TCHAR>(), origin);
        m_nodes.back().import_transitions(nfa.get_state(0), offset);
        for (int i = 1; i < nfa.get_state_num(); i++)
        {
            m_nodes.emplace_back();
            m_nodes.back().import_transitions(nfa.get_state(i), offset);
        }
        return m_nodes.size() - 1;
    }
    int import_nonterminal(const Identifier& id, int origin)
    {
        return add_node(CATNNode<TCHAR>(id), origin);
    }
    int import_atn_node(const ATN<TCHAR>& atn, int index, int origin, std::vector<int>& node_map)
    {
        const ATNNode<TCHAR>& node = atn.get_node(index);

        node_map[index] = m_nodes.size();

        switch (node.type())
        {
        case ATNNodeType::Blank:
            origin = add_node(CharClass<TCHAR>(), origin);
            break;
        case ATNNodeType::LiteralTerminal:
            origin = import_literal_terminal(node.get_literal(), origin);
            break;
        case ATNNodeType::RegularTerminal:
            origin = import_regular_terminal(node.get_nfa(), origin);
            break;
        case ATNNodeType::Nonterminal:
            origin = import_nonterminal(node.get_submachine(), origin);
            break;
        }

        int next = origin;
        for (const auto& tr : node.get_transitions())
        {
            if (node_map[tr.dest()] < 0)
                next = import_atn_node(id, tr.dest(), origin, node_map);
            else
                m_nodes[origin].add_transition(CharClass<TCHAR>(), node_map[tr.dest()]);
        }
        return next;
    }
public:
    CompositeATN(const ATN<TCHAR>& atn)
    {
        std::vector<int> node_map(atn.get_node_num(), -1);

        import_atn_node(atn, 0, -1, node_map);
    }
    virtual ~CompositeATN()
    {
    }
};
}
