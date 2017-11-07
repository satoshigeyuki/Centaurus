#pragma once

#include <unordered_map>
#include <vector>
#include <utility>
#include <string>

#include "atn.hpp"
#include "nfa.hpp"

namespace Centaurus
{
enum class CATNNodeType
{
    Normal,
    Barrier
};

template<typename TCHAR> using CATNTransition = NFATransition<TCHAR>;

template<typename TCHAR>
class CATNNode
{
    CATNNodeType m_type;
    std::vector<CATNTransition<TCHAR> > m_transitions;
public:
    CATNNode()
        : m_type(CATNNodeType::Normal)
    {
    }
    CATNNode(CATNNodeType type)
        : m_type(type)
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
        for (const auto& tr : state.transitions())
        {
            m_transitions.push_back(tr.offset(offset));
        }
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
    std::vector<std::pair<Identifier, int> > m_stack;
    const std::unordered_map<Identifier, ATN<TCHAR> >& m_networks;
    std::vector<CATNNode<TCHAR> > m_nodes;
private:
    int add_node(const CharClass<TCHAR>& cc, int origin)
    {
        if (!m_nodes.empty() && origin >= 0)
            m_nodes[origin].add_transition(CharClass<TCHAR>(), m_nodes.size());
        m_nodes.emplace_back();
        return m_nodes.size() - 1;
    }
    int add_node(const CATNNode<TCHAR>& node)
    {
        m_nodes.push_back(node);
        return m_nodes.size() - 1;
    }
    int import_literal_terminal(const std::basic_string<TCHAR>& literal, int origin)
    {
        for (TCHAR ch : literal)
        {
            origin = add_node(CharClass<TCHAR>(ch, ch + 1));
        }
        return origin;
    }
    int import_regular_terminal(const NFA<TCHAR>& nfa, int origin)
    {
        int offset = m_nodes.size();
        add_node(CharClass<TCHAR>(), origin);
        m_nodes.back().import_transitions(nfa.get_state(0), offset);
        for (unsigned int i = 1; i < nfa.get_state_num(); i++)
        {
            m_nodes.emplace_back();
            m_nodes.back().import_transitions(nfa.get_state(i), offset);
        }
        return m_nodes.size() - 1;
    }
    int import_atn_node(const Identifier& id, int index, int origin, std::vector<int>& node_map)
    {
        const ATNNode<TCHAR>& node = m_networks[id].get_node(index);
        node_map[index] = m_nodes.size();
        switch (node.get_type())
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
            if (std::find_if(m_stack.begin(), m_stack.end(), [&](const std::pair<Identifier, int>& p) -> bool
                {
                    return p.first == node.get_invoke() && p.second == index;
                }) == m_stack.end())
            {
                origin = add_node(CATNNode(CATNNodeType::Barrier));
            }
            else
            {
                m_stack.emplace_back(id, index);
                origin = import_atn(node.get_invoke());
                m_stack.pop_back();
            }
            break;
        }

        int next = origin;
        for (const auto& tr : node.get_transitions())
        {
            if (node_map[tr.dest()] < 0)
                next = import_atn_node(id, tr.dest(), origin, node_map);
        }
        return next;
    }
    int import_atn(const Identifier& id)
    {
        const ATN<TCHAR>& atn = m_networks[id];

        std::vector<int> node_map(atn.m_nodes.size(), -1);

        return import_atn_node(atn, 0, -1, node_map);
    }
public:
    CompositeATN(const std::unordered_map<Identifier, ATN<TCHAR> >& networks, const Identifier& root)
        : m_networks(networks)
    {
        import_atn(root);

        m_stack.clear();
    }
    virtual ~CompositeATN()
    {
    }
    void print_flat(std::ostream& os, const std::string& name) const
    {
        os << "digraph " << name << " {" << std::endl;
        os << "rankdir=\"LR\";" << std::endl;
        os << "graph [ charset=\"UTF-8\" ];" << std::endl;
        os << "node [ style=\"solid,filled\" ];" << std::endl;
        os << "edge [ style=\"solid\" ];" << std::endl;

        for (int i = 0; i < m_nodes.size(); i++)
        {
            os << "N" << i << " [ label=\"" << i << "\" ];" << std::endl;
        }

        for (int i = 0; i < m_nodes.size(); i++)
        {
            m_nodes[i].print(os, i);
        }

        os << "}" << std::endl;
    }
};
}
