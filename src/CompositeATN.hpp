#pragma once

#include <unordered_map>
#include <vector>
#include <array>

#include "Identifier.hpp"
#include "NFA.hpp"
#include "ATN.hpp"
#include "Grammar.hpp"

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
    void add_transition(const CharClass<TCHAR>& cc, int dest, int tag = 0)
    {
        m_transitions.emplace_back(cc, dest, false, tag);
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
	void print(std::wostream& os, int from) const
	{
		if (m_submachine)
		{
			os << L"N" << from << L" [ label=\"" << m_submachine << L"\" ];" << std::endl;
		}
		else
		{
			os << L"N" << from << L" [ label=\"" << from << L"\" ];" << std::endl;
		}

		for (const auto& t : m_transitions)
		{
			os << L"N" << from << L" -> N" << t.dest() << L" [ label=\"";
			os << t.tag() << L":[" << t.label() << L"]";
			os << L"\" ];" << std::endl;
		}
	}
};

template<typename TCHAR>
class CATNMachine
{
    std::vector<CATNNode<TCHAR> > m_nodes;
private:
    int add_node(const CharClass<TCHAR>& cc, int origin, int tag)
    {
        if (!m_nodes.empty() && origin >= 0)
            m_nodes[origin].add_transition(cc, m_nodes.size(), tag);
        m_nodes.emplace_back();
        return m_nodes.size() - 1;
    }
    int add_node(const CATNNode<TCHAR>& node, int origin, int tag)
    {
        if (!m_nodes.empty() && origin >= 0)
            m_nodes[origin].add_transition(CharClass<TCHAR>(), m_nodes.size(), tag);
        m_nodes.push_back(node);
        return m_nodes.size() - 1;
    }
    int import_whitespace(const CharClass<TCHAR>& cc, int origin, int tag)
    {
        origin = add_node(CharClass<TCHAR>(), origin, tag);
        m_nodes[origin].add_transition(cc, origin);
        return origin;
    }
    int import_literal_terminal(const std::basic_string<TCHAR>& literal, int origin, int tag)
    {
        for (TCHAR ch : literal)
        {
            origin = add_node(CharClass<TCHAR>(ch, ch + 1), origin, tag);
            tag = 0;
        }
        return origin;
    }
    int import_regular_terminal(const NFA<TCHAR>& nfa, int origin, int tag)
    {
        int offset = m_nodes.size();
        add_node(CharClass<TCHAR>(), origin, tag);
        m_nodes.back().import_transitions(nfa.get_state(0), offset);
        for (int i = 1; i < nfa.get_state_num(); i++)
        {
            m_nodes.emplace_back();
            m_nodes.back().import_transitions(nfa.get_state(i), offset);
        }
        return m_nodes.size() - 1;
    }
    int import_nonterminal(const Identifier& id, int origin, int tag)
    {
        return add_node(CATNNode<TCHAR>(id), origin, tag);
    }
    void import_atn_node(const ATNMachine<TCHAR>& atn, int index, int origin, std::vector<int>& node_map, int tag)
    {
        const ATNNode<TCHAR>& node = atn.get_node(index);

        node_map[index] = m_nodes.size();

        switch (node.type())
        {
        case ATNNodeType::Blank:
            origin = add_node(CharClass<TCHAR>(), origin, tag);
            break;
        case ATNNodeType::LiteralTerminal:
            origin = import_literal_terminal(node.get_literal(), origin, tag);
            break;
        case ATNNodeType::RegularTerminal:
            origin = import_regular_terminal(node.get_nfa(), origin, tag);
            break;
        case ATNNodeType::Nonterminal:
            origin = import_nonterminal(node.get_submachine(), origin, tag);
            break;
        case ATNNodeType::WhiteSpace:
            origin = import_whitespace(CharClass<TCHAR>({u' ', u'\t', u'\r', u'\n'}), origin, tag);
            break;
        }

        m_nodes[origin].set_source(index);

        for (const auto& tr : node.get_transitions())
        {
            if (node_map[tr.dest()] < 0)
                import_atn_node(atn, tr.dest(), origin, node_map, tr.tag());
            else
                m_nodes[origin].add_transition(CharClass<TCHAR>(), node_map[tr.dest()], tr.tag());
        }
    }
public:
    CATNMachine(const ATNMachine<TCHAR>& atn)
    {
        std::vector<int> node_map(atn.get_node_num(), -1);

        import_atn_node(atn, 0, -1, node_map, 0);
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
	void print(std::wostream& os, const std::wstring& name) const
	{
		os << L"digraph " << name << L" {" << std::endl;
		os << L"rankdir=\"LR\";" << std::endl;
		os << L"graph [ charset=\"UTF-8\" ];" << std::endl;
		os << L"node [ style=\"solid,filled\" ];" << std::endl;
		os << L"edge [ style=\"solid\" ];" << std::endl;

		for (unsigned int i = 0; i < m_nodes.size(); i++)
		{
			m_nodes[i].print(os, i);
		}

		os << L"}" << std::endl;
	}
};

class CATNClosureElement
{
    ATNPath m_path;
    int m_color;
    PriorityChain m_priority;
public:
    CATNClosureElement(const ATNPath& path, int color)
        : m_path(path), m_color(color)
    {
    }
    CATNClosureElement(const ATNPath& path, int color, const PriorityChain& priority)
        : m_path(path), m_color(color), m_priority(priority)
    {
    }
    const ATNPath& path() const
    {
        return m_path;
    }
    int color() const
    {
        return m_color;
    }
    const PriorityChain& priority() const
    {
        return m_priority;
    }
    bool operator<(const CATNClosureElement& e) const
    {
        int path_cmp = m_path.compare(e.m_path);

        return path_cmp != 0 ? path_cmp < 0 : m_color < e.m_color;
    }
    bool operator==(const CATNClosureElement& e) const
    {
        int path_cmp = m_path.compare(e.m_path);

        return path_cmp == 0 && m_color == e.m_color;
    }
};

//using CATNClosure = std::set<std::pair<ATNPath, int> >;
using CATNClosure = std::set<CATNClosureElement>;

static std::wostream& operator<<(std::wostream& os, const CATNClosure& closure)
{
    for (const auto& p : closure)
    {
        os << p.path() << L':' << p.color() << std::endl;
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
                if (q.color() != prev)
                {
                    if (prev > 0) return false;
                    prev = q.color();
                }
            }
        }
        return true;
    }
};

template<typename TCHAR>
std::wostream& operator<<(std::wostream& os, const CATNDepartureSet<TCHAR>& deptset)
{
	for (const auto& p : deptset)
	{
		os << p.first << L" -> ";
		for (const auto& q : p.second)
		{
			os << q.path() << L":" << q.color() << L" ";
		}
		os << std::endl;
	}
	return os;
}

template<typename TCHAR>
class CATNDeparture
{
    CharClass<TCHAR> m_label;
    ATNPath m_path;
    int m_color;
    PriorityChain m_priority;
public:
    CATNDeparture(const CharClass<TCHAR>& label, const ATNPath& path, int color)
        : m_label(label), m_path(path), m_color(color)
    {
    }
    const CharClass<TCHAR>& label() const
    {
        return m_label;
    }
    const ATNPath& path() const
    {
        return m_path;
    }
    int color() const
    {
        return m_color;
    }
};

template<typename TCHAR>
class CATNDepartureSetFactory
{
    std::vector<CATNDeparture<TCHAR> > m_departures;
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
            IndexVector borders_for_one_tr = t.label().collect_borders();

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
                if (t.label().includes(atomic_range))
                {
                    closure.emplace(t.path(), t.color());
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
    void build_wildcard_closure(CATNClosure& closure, const Identifier& id, int color, ATNStateStack& stack, const PriorityChain& priority) const;
    /*!
     * @brief Add all the CATN nodes reachable from a path
     */
    void build_closure_exclusive(CATNClosure& closure, const ATNPath& path, int color, ATNStateStack& stack, const PriorityChain& priority) const;
    void build_closure_inclusive(CATNClosure& closure, const ATNPath& path, int color, ATNStateStack& stack, const PriorityChain& priority) const;
    void build_wildcard_departure_set(CATNDepartureSetFactory<TCHAR>& deptset_factory, const Identifier& id, int color, ATNStateStack& stack) const;
    void build_departure_set_r(CATNDepartureSetFactory<TCHAR>& deptset_factory, const ATNPath& path, int color, ATNStateStack& stack) const;
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
            build_closure_inclusive(ret, p.path(), p.color(), stack, p.priority());
        }

        return ret;
    }
    CATNClosure build_closure(const ATNPath& path, int color) const
    {
        CATNClosure closure;

        ATNStateStack stack;

        build_closure_inclusive(closure, path, color, stack, PriorityChain());

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

            build_closure_inclusive(closure, path.replace_index(transitions[i].dest()), i + 1, stack, PriorityChain(path.leaf_id(), path.leaf_index(), transitions[i].tag()));
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
            build_departure_set_r(deptset_factory, p.path(), p.color(), stack);
        }

        return deptset_factory.build_departure_set();
    }
	/*void print(std::wostream& os)
	{
		for (const auto& machine : m_dict)
		{
			machine.second.print(os, machine.first);
		}
	}*/
};
}
