#pragma once

#include <unordered_map>
#include <vector>
#include <array>

#include "Identifier.hpp"
#include "NFA.hpp"
#include "ATN.hpp"
#include "Grammar.hpp"

namespace std
{
template<> struct less<std::pair<Centaurus::ATNPath, int> >
{
    bool operator()(const std::pair<Centaurus::ATNPath, int>& x, const std::pair<Centaurus::ATNPath, int>& y) const
    {
        int path_cmp = x.first.compare(y.first);

        return path_cmp != 0 ? path_cmp < 0 : x.second < y.second;
    }
};
}

namespace Centaurus
{
template<typename TCHAR> using CATNTransition = NFATransition<TCHAR>;
template<typename TCHAR>
class CATNNode
{
    std::vector<CATNTransition<TCHAR> > m_transitions;
    Identifier m_submachine;
    int m_source;
public:
    CATNNode()
        : m_source(-1)
    {
    }
    CATNNode(const Identifier& id)
        : m_submachine(id), m_source(-1)
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
        //operator Identifier::bool() returns true if the identifier has non-zero length
        return !m_submachine;
    }
    const Identifier& get_submachine() const
    {
        //Returns an empty Identifier if the node is terminal
        return m_submachine;
    }
    void set_source(int source)
    {
        m_source = source;
    }
    int get_source() const
    {
        return m_source;
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
    int import_whitespace(const CharClass<TCHAR>& cc, int origin)
    {
        origin = add_node(CharClass<TCHAR>(), origin);
        m_nodes[origin].add_transition(cc, origin);
        return origin;
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
    void import_atn_node(const ATNMachine<TCHAR>& atn, int index, int origin, std::vector<int>& node_map)
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
        case ATNNodeType::WhiteSpace:
            origin = import_whitespace(CharClass<TCHAR>({u' ', u'\t', u'\r', u'\n'}), origin);
            break;
        }

        m_nodes[origin].set_source(index);

        for (const auto& tr : node.get_transitions())
        {
            if (node_map[tr.dest()] < 0)
                import_atn_node(atn, tr.dest(), origin, node_map);
            else
                m_nodes[origin].add_transition(CharClass<TCHAR>(), node_map[tr.dest()]);
        }
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
    const CATNNode<TCHAR>& operator[](int index) const
    {
        return m_nodes.at(index);
    }
    int get_num_nodes() const
    {
        return m_nodes.size();
    }
    int convert_atn_index(int index) const
    {
        for (int i = 0; i < m_nodes.size(); i++)
        {
            if (m_nodes[i].get_source() == index) return i;
        }
        return -1;
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

static std::ostream& operator<<(std::ostream& os, const CATNClosure& closure)
{
    for (const auto& p : closure)
    {
        os << p.first << ":" << p.second << std::endl;
    }
    return os;
}

template<typename TCHAR>
class CATNDepartureSet : public std::vector<std::pair<CharClass<TCHAR>, CATNClosure> >
{
public:
    CATNDepartureSet()
    {
    }
    virtual ~CATNDepartureSet()
    {
    }
    /*!
     * @brief Check if this Departure Set is considered resolved.
     *
     * The check is performed by searching for duplicate colors in a single element.
     */
    bool is_resolved() const
    {
        for (const auto& p : *this)
        {
            int prev = 0;

            for (const auto& q : p.second)
            {
                if (q.second != prev)
                {
                    if (prev > 0) return false;
                    prev = q.second;
                }
            }
        }
        return true;
    }
};

template<typename TCHAR>
std::ostream& operator<<(std::ostream& os, const CATNDepartureSet<TCHAR>& deptset)
{
    for (const auto& p : deptset)
    {
        os << p.first << " -> ";
        for (const auto& q : p.second)
        {
            os << q.first << ":" << q.second << " ";
        }
        os << std::endl;
    }
    return os;
}

template<typename TCHAR>
class CATNDepartureSetFactory
{
    std::vector<std::tuple<CharClass<TCHAR>, ATNPath, int> > m_departures;
public:
    CATNDepartureSetFactory()
    {

    }
    virtual ~CATNDepartureSetFactory()
    {

    }
    void add(const CharClass<TCHAR>& cc, const ATNPath& path, int color)
    {
        m_departures.emplace_back(cc, path, color);
    }
    CATNDepartureSet<TCHAR> build_departure_set()
    {
        std::set<int> borders;

        for (const auto& t : m_departures)
        {
            IndexVector borders_for_one_tr = std::get<0>(t).collect_borders();

            borders.insert(borders_for_one_tr.cbegin(), borders_for_one_tr.cend());
        }

        CATNDepartureSet<TCHAR> deptset;

        for (auto i = borders.cbegin(); i != borders.cend();)
        {
            TCHAR atomic_range_start = *i;
            if (++i == borders.cend()) break;
            TCHAR atomic_range_end = *i;

            Range<TCHAR> atomic_range(atomic_range_start, atomic_range_end);

            CATNClosure closure;

            for (const auto& t : m_departures)
            {
                if (std::get<0>(t).includes(atomic_range))
                {
                    closure.emplace(std::get<1>(t), std::get<2>(t));
                }
            }

            if (!closure.empty())
            {
                deptset.emplace_back(atomic_range, closure);
            }
        }
        return deptset;
    }
};

template<typename TCHAR>
class CompositeATN
{
    std::unordered_map<Identifier, CATNMachine<TCHAR> > m_dict;
private:
    /*!
     * @brief Add all the CATN nodes reachable from all the invocation sites of a machine
     */
    void build_wildcard_closure(CATNClosure& closure, const Identifier& id, int color, ATNStateStack& stack) const
    {
        //std::cout << "Wildcard " << id.narrow() << ":" << color << std::endl;

        for (const auto& p : m_dict)
        {
            for (int i = 0; i < p.second.get_num_nodes(); i++)
            {
                const CATNNode<TCHAR>& node = p.second[i];

                if (node.get_submachine() == id)
                {
                    if (stack.find(p.first, i))
                    {
                        //std::cout << "Upward sentinel reached." << std::endl;
                        continue;
                    }
                    stack.push(p.first, i);
                    build_closure_exclusive(closure, ATNPath(p.first, i), color, stack);
                    stack.pop();
                }
            }
        }
    }
    /*!
     * @brief Add all the CATN nodes reachable from a path
     */
    void build_closure_exclusive(CATNClosure& closure, const ATNPath& path, int color, ATNStateStack& stack) const
    {
        const CATNNode<TCHAR>& node = get_node(path);

        //std::cout << "Exclusive " << path << ":" << color << std::endl;

        if (node.is_stop_node())
        {
            //std::cout << "Stop node " << path << std::endl;

            ATNPath parent = path.parent_path();

            if (parent.depth() == 0)
            {
                build_wildcard_closure(closure, path.leaf_id(), color, stack);
            }
            else
            {
                build_closure_exclusive(closure, parent, color, stack);
            }
        }
        else
        {
            //std::cout << "Intermediate node " << path << std::endl;

            for (const auto& tr : node.get_transitions())
            {
                if (tr.is_epsilon())
                {
                    ATNPath dest_path = path.replace_index(tr.dest());

                    const CATNNode<TCHAR>& dest_node = get_node(dest_path);

                    if (dest_node.is_terminal())
                    {
                        closure.emplace(dest_path, color);

                        build_closure_exclusive(closure, dest_path, color, stack);
                    }
                    else
                    {
                        build_closure_inclusive(closure, dest_path.add(dest_node.get_submachine(), 0), color, stack);
                    }
                }
            }
        }
    }
    void build_closure_inclusive(CATNClosure& closure, const ATNPath& path, int color, ATNStateStack& stack) const
    {
        const CATNNode<TCHAR>& node = get_node(path);

        if (node.is_terminal())
        {
            closure.emplace(path, color);

            build_closure_exclusive(closure, path, color, stack);
        }
        else
        {
            build_closure_inclusive(closure, path.add(node.get_submachine(), 0), color, stack);
        }
    }
    void build_wildcard_departure_set(CATNDepartureSetFactory<TCHAR>& deptset_factory, const Identifier& id, int color, ATNStateStack& stack) const
    {
        for (const auto& p : m_dict)
        {
            for (int i = 0; i < p.second.get_num_nodes(); i++)
            {
                if (p.second[i].get_submachine() == id)
                {
                    if (stack.find(p.first, i))
                    {
                        //std::cout << "Upward sentinel reached." << std::endl;
                        continue;
                    }
                    stack.push(p.first, i);
                    build_departure_set_r(deptset_factory, ATNPath(p.first, i), color, stack);
                    stack.pop();
                }
            }
        }
    }
    void build_departure_set_r(CATNDepartureSetFactory<TCHAR>& deptset_factory, const ATNPath& path, int color, ATNStateStack& stack) const
    {
        const CATNNode<TCHAR>& node = get_node(path);

        if (node.is_stop_node())
        {
            ATNPath parent_path = path.parent_path();

            if (parent_path.depth() == 0)
            {
                build_wildcard_departure_set(deptset_factory, path.leaf_id(), color, stack);
            }
            else
            {
                build_departure_set_r(deptset_factory, parent_path, color, stack);
            }
        }
        else
        {
            for (const auto& tr : node.get_transitions())
            {
                if (!tr.is_epsilon())
                {
                    deptset_factory.add(tr.label(), path.replace_index(tr.dest()), color);
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
        return m_dict.at(path.leaf_id())[path.leaf_index()];
    }
    const CATNNode<TCHAR>& get_node(const ATNPath& path) const
    {
        return m_dict.at(path.leaf_id())[path.leaf_index()];
    }
    const CATNNode<TCHAR>& get_node(const Identifier& id, int index) const
    {
        return m_dict.at(id)[index];
    }
    const std::vector<CATNTransition<TCHAR> >& get_transitions(const ATNPath& path) const
    {
        return get_node(path).get_transitions();
    }
    const ATNPath convert_atn_path(const ATNPath& path) const
    {
        const CATNMachine<TCHAR>& machine = m_dict.at(path.leaf_id());

        return path.replace_index(machine.convert_atn_index(path.leaf_index()));
    }
    CATNClosure build_closure(const CATNClosure& closure) const
    {
        CATNClosure ret;

        ATNStateStack stack;

        for (const auto& p : closure)
        {
            build_closure_inclusive(ret, p.first, p.second, stack);
        }

        return ret;
    }
    CATNClosure build_closure(const ATNPath& path, int color) const
    {
        CATNClosure closure;

        ATNStateStack stack;

        build_closure_inclusive(closure, path, color, stack);

        return closure;
    }
    /*!
     * @brief Construct a closure around the root path (decision point)
     */
    CATNClosure build_root_closure(const ATNPath& path) const
    {
        CATNClosure closure;

        ATNStateStack stack;

        const CATNNode<TCHAR>& root_node = get_node(path);

        const std::vector<CATNTransition<TCHAR> >& transitions = root_node.get_transitions();

        for (unsigned int i = 0; i < transitions.size(); i++)
        {
            //All transitions originating from the root node must be epsilon transitions
            assert(transitions[i].is_epsilon());

            build_closure_inclusive(closure, path.replace_index(transitions[i].dest()), i + 1, stack);
        }

        return closure;
    }
    /*!
     * @brief Collect all the outbound transitions from the closure
     */
    CATNDepartureSet<TCHAR> build_departure_set(const CATNClosure& closure) const
    {
        CATNDepartureSetFactory<TCHAR> deptset_factory;

        ATNStateStack stack;

        for (const auto& p : closure)
        {
            build_departure_set_r(deptset_factory, p.first, p.second, stack);
        }

        return deptset_factory.build_departure_set();
    }
};
}
