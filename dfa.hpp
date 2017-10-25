#pragma once

#include <vector>
#include <limits>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <assert.h>
#include <locale>
#include <codecvt>

#include "util.hpp"
#include "exception.hpp"

namespace Centaur
{
template<typename TCHAR> class Range
{
    TCHAR m_start, m_end;
public:
    Range(TCHAR start, TCHAR end)
        : m_start(start), m_end(end)
    {
        assert(start < end);
    }
    Range(wchar_t start, wchar_t end)
    {
        assert(start < end);

        m_start = wide_to_target<TCHAR>(start);
        m_end = wide_to_target<TCHAR>(end);
    }
    Range(const Range<TCHAR>& r)
        : m_start(r.m_start), m_end(r.m_end)
    {
    }
    virtual ~Range()
    {
    }
    bool operator<(const Range<TCHAR>& r) const
    {
        return m_end <= r.start;
    }
    bool operator>(const Range<TCHAR>& r) const
    {
        return r.end <= m_start;
    }
    bool overlaps(const Range<TCHAR>& r) const
    {
        return m_start < r.end && r.start < m_end;
    }
    TCHAR start() const
    {
        return m_start;
    }
    TCHAR end() const
    {
        return m_end;
    }
};

template<typename TCHAR> class NFACharClass
{
    std::vector<Range<TCHAR> > m_ranges;
public:
    NFACharClass()
    {
    }
    NFACharClass(wchar_t start, wchar_t end)
    {
        m_ranges.push_back(Range<TCHAR>(start, end));
    }
    NFACharClass(const NFACharClass<TCHAR>& cc)
        : m_ranges(cc.m_ranges)
    {
    }
    virtual ~NFACharClass()
    {
    }
    static NFACharClass<TCHAR> make_star()
    {
        return NFACharClass<TCHAR>(0, std::numeric_limits<TCHAR>::max());
    }
    bool is_epsilon() const
    {
        return m_ranges.empty();
    }
    NFACharClass<TCHAR> operator~() const
    {
        NFACharClass<TCHAR> new_class;

        TCHAR begin = 0;

        auto i = m_ranges.cbegin();

        for (; i != m_ranges.cend(); i++)
        {
            if (i->start() > begin)
            {
                new_class.m_ranges.emplace_back(begin, i->start());
            }
            begin = i->end();
        }

        if (begin < std::numeric_limits<TCHAR>::max())
        {
            new_class.m_ranges.emplace_back(begin, std::numeric_limits<TCHAR>::max());
        }
        return new_class;
    }
    NFACharClass<TCHAR> operator|(const Range<TCHAR>& r) const
    {
        NFACharClass<TCHAR> new_class;

        auto i = m_ranges.cbegin();

        for (; i != m_ranges.cend(); i++)
        {
            if (i->overlaps(r))
            {
                new_class.m_ranges.push_back(i->merge(r));
                i++;
                break;
            }
            else
            {
                new_class.m_ranges.push_back(*i);
            }
        }
        new_class.m_ranges.insert(new_class.m_ranges.end(), i, m_ranges.cend());

        return new_class;
    }
    NFACharClass<TCHAR>& operator|=(const Range<TCHAR>& r)
    {
        return *this = *this | r;
    }
    void print(std::ostream& os) const
    {
        auto i = m_ranges.cbegin();

        for (; i != m_ranges.cend();)
        {
            if (i->end() == i->start() + 1)
            {
                if (i->start() == wide_to_target<TCHAR>(L'"'))
                    os << "\\\"";
                else
                    os << i->start();
            }
            else
            {
                if (i->start() == wide_to_target<TCHAR>(L'"'))
                    os << "\\\"";
                else
                    os << i->start();
                os << '-';
                if (i->end() == wide_to_target<TCHAR>(L'"'))
                    os << "\\\"";
                else
                    os << i->end();
            }
            i++;
        }
    }
};

template<typename TCHAR> class NFATransition
{
    NFACharClass<TCHAR> m_label;
    int m_dest;
public:
    NFATransition(const NFACharClass<TCHAR>& label, int dest)
        : m_label(label), m_dest(dest)
    {
    }
    NFATransition(const NFATransition<TCHAR>& transition)
        : m_label(transition.m_label), m_dest(transition.m_dest)
    {
    }
    virtual ~NFATransition()
    {
    }
    const NFACharClass<TCHAR>& label() const
    {
        return m_label;
    }
    int dest() const
    {
        return m_dest;
    }
    NFATransition<TCHAR> offset(int value) const
    {
        return NFATransition<TCHAR>(m_label, m_dest + value);
    }
};

template<typename TCHAR> class NFA;

template<typename TCHAR> class NFAState
{
    std::vector<NFATransition<TCHAR> > m_transitions;
    NFA<TCHAR>& m_parent;
public:
    NFAState(NFA<TCHAR>& parent)
        : m_parent(parent)
    {
    }
    NFAState(const NFAState<TCHAR>& state)
        : m_transitions(state.m_transitions), m_parent(state.m_parent)
    {
    }
    virtual ~NFAState()
    {
    }
    void add_transition(const NFATransition<TCHAR>& transition)
    {
        m_transitions.push_back(transition);
    }
    void print_transitions(std::ostream& os, int from) const
    {
        for (const auto& t : m_transitions)
        {
            os << "S" << from << " -> " << "S" << t.dest() << " [ label=\"";
            t.label().print(os);
            os << "\" ];" << std::endl;
        }
    }
    NFAState<TCHAR> offset(NFA<TCHAR>& new_parent, int value) const
    {
        NFAState<TCHAR> new_state(new_parent);

        for (const auto& transition : m_transitions)
        {
            new_state.add_transition(transition.offset(value));
        }

        return new_state;
    }
    void rebase_transitions(const NFAState<TCHAR>& src, int offset_value)
    {
        for (const auto& i : src.m_transitions)
        {
            add_transition(i.offset(offset_value));
        }
    }
};

template<typename TCHAR> class NFA
{
    std::vector<NFAState<TCHAR> > m_states;
public:
    /*!
     * @brief Concatenate another NFA to the final state
     */
    void concat(const NFA<TCHAR>& nfa)
    {
        int initial_size = m_states.size();

        auto i = nfa.m_states.cbegin().advance();

        for (; i != nfa.m_states.cend(); i++)
        {
            //Copy all states except the initial state
            m_states.push_back(i->offset(*this, initial_size - 1));
        }

        //Rebase the transitions from the start state to the newly added states
        m_states[initial_size - 1].rebase_transitions(nfa.m_states[0], initial_size - 1);
    }
    /*!
     * @brief Form a selection with another NFA
     */
    void select(const NFA<TCHAR>& nfa)
    {
        int initial_size = m_states.size();

        auto i = nfa.m_states.cbegin().advance();

        for (; i != nfa.m_states.cend(); i++)
        {
            //Copy all states except the initial state
            m_states.push_back(i->offset(*this, initial_size - 1));
        }

        //Rebase the transitions from the start state to the newly added states
        m_states[0].rebase_transitions(nfa.m_states[0], initial_size - 1);

        //Add the join state
        add_state(NFACharClass<TCHAR>());

        add_transition_from(NFACharClass<TCHAR>(), initial_size - 1);
    }
    NFA()
    {
        m_states.emplace_back(*this);
    }
    virtual ~NFA()
    {
    }
    /*!
     * @brief Add a transition to a new state from the final state.
     */
    int add_state(const NFACharClass<TCHAR>& cc)
    {
        m_states.back().add_transition(cc, m_states.size() - 1);
        m_states.emplace_back();
        return ret;
    }
    /*!
     * @brief Add a transition from the final state to another state.
     */
    void add_transition_to(const NFACharClass<TCHAR>& cc, int dest)
    {
        m_states.back().add_transition(cc, dest);
    }
    /*!
     * @brief Add a transition to the final state from another state.
     */
    void add_transition_from(const NFACharClass<TCHAR>& cc, int src)
    {
        m_states[src].add_transition(cc, m_states.size() - 1);
    }
    /*!
     * @brief Print out the graph structure into Graphviz file.
     */
    void print_graph(const std::string& file_name, const std::string& graph_name)
    {
        std::ofstream of(file_name);

        of << "digraph " << graph_name << "{" << std::endl;

        of << "graph [ charset=\"UTF-8\", style=\"filled\" ];" << std::endl;

        of << "node [ style=\"solid,filled\" ];" << std::endl;

        of << "edge [ style=\"solid\" ];" << std::endl;

        for (int i = 0; i < m_states.size(); i++)
        {
            of << "S" << i << " [shape = circle];" << std::endl;
        }

        for (int i = 0; i < m_states.size(); i++)
        {
            m_states[i].print_transitions(of, i);
        }

        of << "}" << std::endl;
    }
};
}
