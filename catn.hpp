#pragma once

#include <unordered_map>
#include <vector>
#include <utility>
#include <string>
#include <algorithm>

#include "atn.hpp"
#include "nfa.hpp"
#include "identifier.hpp"
#include "util.hpp"

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
    ATNPath m_path;
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
    CATNNodeType get_type() const
    {
        return m_type;
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
    int import_atn_node(const Identifier& id, int index, int origin, std::vector<int>& node_map)
    {
        const ATNNode<TCHAR>& node = m_networks.at(id).get_node(index);
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
            if (std::find_if(m_stack.begin(), m_stack.end(), [&](const std::pair<Identifier, int>& p) -> bool
                {
                    return p.first == id && p.second == index;
                }) != m_stack.end())
            {
                origin = add_node(CATNNode<TCHAR>(CATNNodeType::Barrier), origin);
            }
            else
            {
                m_stack.emplace_back(id, index);
                origin = import_atn(node.get_invoke(), origin);
                m_stack.pop_back();
            }
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
    int import_atn(const Identifier& id, int origin)
    {
        const ATN<TCHAR>& atn = m_networks.at(id);

        std::vector<int> node_map(atn.get_node_num(), -1);

        import_atn_node(id, 0, origin, node_map);

        return node_map.back();
    }
public:
    CompositeATN(const std::unordered_map<Identifier, ATN<TCHAR> >& networks, const Identifier& root)
        : m_networks(networks)
    {
        import_atn(root, -1);

        m_stack.clear();
    }
    virtual ~CompositeATN()
    {
    }
    void epsilon_closure_sub(std::set<int>& closure, int origin) const
    {
        closure.insert(origin);
        for (const auto& t : m_nodes[origin].get_transitions())
        {
            if (t.is_epsilon())
            {
                epsilon_closure_sub(closure, t.dest());
            }
        }
    }
    std::set<int> epsilon_closure(int origin) const
    {
        std::set<int> closure;

        epsilon_closure_sub(closure, origin);

        return closure;
    }
    std::set<int> epsilon_closure(const std::set<int>& origins) const
    {
        std::set<int> closure;

        for (int origin : origins)
        {
            epsilon_closure_sub(closure, origin);
        }

        return closure;
    }
    void print_flat(std::ostream& os, const std::string& name) const
    {
        os << "digraph " << name << " {" << std::endl;
        os << "rankdir=\"LR\";" << std::endl;
        os << "graph [ charset=\"UTF-8\" ];" << std::endl;
        os << "node [ style=\"solid,filled\", color=\"black\" ];" << std::endl;
        os << "edge [ style=\"solid\" ];" << std::endl;

        for (unsigned int i = 0; i < m_nodes.size(); i++)
        {
            switch (m_nodes[i].get_type())
            {
            case CATNNodeType::Normal:
                os << "N" << i << " [ fillcolor=\"white\", label=\"" << i << "\" ];" << std::endl;
                break;
            case CATNNodeType::Barrier:
                os << "N" << i << " [ fillcolor=\"grey\", label=\"" << i << "\" ];" << std::endl;
                break;
            }
        }

        for (unsigned int i = 0; i < m_nodes.size(); i++)
        {
            m_nodes[i].print(os, i);
        }

        os << "}" << std::endl;
    }
};
}
