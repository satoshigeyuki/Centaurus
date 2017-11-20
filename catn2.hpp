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
    bool is_stop_node() const
    {
        return m_transitions.empty();
    }
    bool is_terminal() const
    {
        return m_submachine;
    }
    const Identifier& get_submachine() const
    {
        return m_submachine;
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
    const CATNNode& operator[](int index) const
    {
        return m_nodes.at(index);
    }
    int get_num_nodes() const
    {
        return m_nodes.size();
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

using CATNClosure = std::set<std::pair<ATNPath, int> >;

template<typename TCHAR>
class CompositeATN
{
    std::unordered_map<Identifier, CATNMachine<TCHAR> > m_dict;
private:
    /*!
     * @brief Add all the CATN nodes reachable from all the invocation sites of a machine
     */
    void build_wildcard_closure(CATNClosure& closure, const Identifier& id, int color) const
    {
        for (const auto& p : m_dict)
        {
            for (unsigned int i = 0; i < p.second.get_num_nodes(); i++)
            {
                const CATNNode<TCHAR>& node = p.second[i];

                if (node.get_submachine() == id)
                {
                    build_closure_sub(closure, path.add(p.first, i), color);
                }
            }
        }
    }
    /*!
     * @brief Add all the CATN nodes reachable from a path
     */
    void build_closure_sub(CATNClosure& closure, const ATNPath& path, int color) const
    {
        const CATNNode<TCHAR>& node = get_node(path);

        if (node.is_stop_node())
        {
            ATNPath parent = path.parent_path();

            if (parent.depth() == 0)
            {
                build_wildcard_closure(closure, path.leaf_id(), color);
            }
            else
            {
                build_closure_sub(closure, parent, color);
            }
        }
        else
        {
            for (const auto& tr : node.get_transitions())
            {
                if (tr.is_epsilon())
                {
                    ATNPath dest_path = path.replace_index(tr.dest());

                    const CATNNode<TCHAR>& dest_node = get_node(dest_path);

                    if (dest_node.is_terminal())
                    {
                        closure.emplace(dest_path, color);

                        build_closure_sub(closure, dest_path, color);
                    }
                    else
                    {
                        build_closure_sub(closure, dest_path.add(dest_node.get_submachine(), 0), color);
                    }
                }
            }
        }
    }
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
    const CATNNode<TCHAR>& get_node(const ATNPath& path) const
    {
        return m_dict.at(path.leaf_id()).get_node(path.leaf_index());
    }
    const CATNNode<TCHAR>& get_node(const Identifier& id, int index) const
    {
        return m_dict.at(id).get_node(index);
    }
    CATNClosure build_closure(const CATNClosure& closure) const
    {
    }
    CATNClosure build_closure(const ATNPath& path, int color) const
    {

    }
};
}
