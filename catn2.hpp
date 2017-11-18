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
        if (m_submachine)
        {
            os << "N" << from << " [ label=\"" << m_submachine.narrow() << "\" ];" << std::endl;
        }
        else
        {
            os << "N" << from << " [ label=\"" << from << "\" ];" << std::endl;
        }

        for (const auto& t : m_transitions)
        {
            os << "N" << from << " -> N" << t.dest() << " [ label=\"";
            os << t.label();
            os << "\" ];" << std::endl;
        }
    }
};

template<typename TCHAR>
class CATNMachine
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
    int import_atn_node(const ATNMachine<TCHAR>& atn, int index, int origin, std::vector<int>& node_map)
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
                next = import_atn_node(atn, tr.dest(), origin, node_map);
            else
                m_nodes[origin].add_transition(CharClass<TCHAR>(), node_map[tr.dest()]);
        }
        return next;
    }
public:
    CATNMachine(const ATNMachine<TCHAR>& atn)
    {
        std::vector<int> node_map(atn.get_node_num(), -1);

        import_atn_node(atn, 0, -1, node_map);
    }
    virtual ~CATNMachine()
    {
    }
    void print(std::ostream& os, const std::string& name) const
    {
        os << "digraph " << name << " {" << std::endl;
        os << "rankdir=\"LR\";" << std::endl;
        os << "graph [ charset=\"UTF-8\" ];" << std::endl;
        os << "node [ style=\"solid,filled\" ];" << std::endl;
        os << "edge [ style=\"solid\" ];" << std::endl;

        for (unsigned int i = 0; i < m_nodes.size(); i++)
        {
            m_nodes[i].print(os, i);
        }

        os << "}" << std::endl;
    }
};

template<typename TCHAR>
class CompositeATN
{
    std::unordered_map<Identifier, CATNMachine<TCHAR> > m_dict;
public:
    CompositeATN(const Grammar<TCHAR>& grammar)
    {
        for (const auto& p : grammar.get_machines())
        {
            m_dict.emplace(p.first, CATNMachine<TCHAR>(p.second));
        }
    }
    virtual ~CompositeATN()
    {
    }
    const CATNMachine<TCHAR>& operator[](const Identifier& id) const
    {
        return m_dict.at(id);
    }
    const CATNNode<TCHAR>& operator[](const ATNPath& path) const
    {
        return m_dict.at(path.leaf_id()).get_node(path.leaf_index());
    }
};
}
