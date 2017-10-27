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
#include <algorithm>

#include "util.hpp"
#include "exception.hpp"
#include "index_vector.hpp"

namespace Centaur
{
/*!
 * @brief Represents an half-open range of type TCHAR.
 *        This class is unable to handle the maximum character of type TCHAR, because of the half-open representation.
 *        This limitation should not be problematic because most character sets reserve that code point.
 */
template<typename TCHAR> class Range
{
    TCHAR m_start, m_end;
public:
    Range(TCHAR start, TCHAR end)
        : m_start(start), m_end(end)
    {
        assert(start < end);
    }
    /*Range(wchar_t start, wchar_t end)
    {
        assert(start < end);

        m_start = wide_to_target<TCHAR>(start);
        m_end = wide_to_target<TCHAR>(end);
    }*/
    static Range<TCHAR> make_from_wide(wchar_t start, wchar_t end)
    {
        assert(start < end);

        return Range<TCHAR>(wide_to_target<TCHAR>(start), wide_to_target<TCHAR>(end));
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
        return m_end < r.m_start;
    }
    bool operator>(const Range<TCHAR>& r) const
    {
        return r.m_end < m_start;
    }
    bool overlaps(const Range<TCHAR>& r) const
    {
        return m_start <= r.m_end && r.m_start <= m_end;
    }
    Range<TCHAR> merge(const Range<TCHAR>& r) const
    {
        return Range<TCHAR>(std::min(m_start, r.m_start), std::max(m_end, r.m_end));
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
        m_ranges.push_back(Range<TCHAR>::make_from_wide(start, end));
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
    void invert()
    {
        TCHAR last_end = 0;

        for (auto& i : m_ranges)
        {
            last_end = i.end();
            i = Range<TCHAR>(last_end, i.start());
        }

        if (last_end < std::numeric_limits<TCHAR>::max())
            m_ranges.emplace_back(last_end, std::numeric_limits<TCHAR>::max());
    }
    NFACharClass<TCHAR> operator~() const
    {
        NFACharClass<TCHAR> cc(*this);

        cc.invert();

        return cc;
    }
    NFACharClass<TCHAR> operator|(Range<TCHAR> r) const
    {
        NFACharClass<TCHAR> cc;

        auto i = m_ranges.cbegin();

        for (; i != m_ranges.cend() && !i->overlaps(r); i++)
            ;
        
        cc.m_ranges.insert(cc.m_ranges.end(), m_ranges.cbegin(), i);

        for (; i != m_ranges.cend() && i->overlaps(r); i++)
        {
            r = r.merge(*i);
        }

        cc.m_ranges.push_back(r);

        cc.m_ranges.insert(cc.m_ranges.end(), i, m_ranges.cend());

        return cc;
    }
    NFACharClass<TCHAR>& operator|=(const Range<TCHAR>& r)
    {
        return *this = *this | r;
    }
    NFACharClass<TCHAR>& operator|(const NFACharClass<TCHAR>& cc) const
    {
        NFACharClass<TCHAR> new_class;

        auto i = m_ranges.cbegin();
        auto j = cc.m_ranges.cbegin();

        Range<TCHAR> ri, rj;

        if (i != m_ranges.cend()) ri = *i;
        if (j != cc.m_ranges.cend()) rj = *j;

        while (i != m_ranges.cend() && j != cc.m_ranges.cend())
        {
            if (ri < rj)
            {
                new_class.m_ranges.push_back(ri);
                if (++i != m_ranges.cend())
                    ri = *i;
            }
            else if (ri > rj)
            {
                new_class.m_ranges.push_back(rj);
                if (++j != cc.m_ranges.cend())
                    rj = *j;
            }
            else
            {
                if (ri.end() < rj.end())
                {
                    rj = rj.merge(ri);
                    if (++i != m_ranges.cend())
                        ri = *i;
                }
                else
                {
                    ri = ri.merge(rj);
                    if (++j != cc.m_ranges.cend())
                        rj = *j;
                }
            }
        }

        return new_class;
    }
    NFACharClass<TCHAR>& operator|=(const NFACharClass<TCHAR>& cc)
    {
        return *this = *this | cc;
    }
    /*!
     * @brief Return a soup of all the boundaries in the class.
     */
    IndexVector collect_borders() const
    {
        IndexVector ret(2 * m_ranges.size(), true);

        for (const auto& r : m_ranges)
        {
            ret.push_back(r.start());
            ret.push_back(r.end());
        }

        return ret;
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
                    os << os.narrow(i->start(), '@');
            }
            else
            {
                if (i->start() == wide_to_target<TCHAR>(L'"'))
                    os << "\\\"";
                else
                    os << os.narrow(i->start(), '@');
                os << '-';
                if (i->end() == wide_to_target<TCHAR>(L'"'))
                    os << "\\\"";
                else
                    os << os.narrow(i->end(), '@');
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
    bool is_epsilon() const
    {
        return m_label.is_epsilon();
    }
};

template<typename TCHAR> class NFA;

template<typename TCHAR, typename TLABEL> class NFABaseState
{
    std::vector<NFATransition<TCHAR> > m_transitions;
    TLABEL m_label;
public:
    NFABaseState()
    {
    }
    NFABaseState(const TLABEL& label)
        : m_label(label)
    {
    }
    NFABaseState(const NFABaseState<TCHAR, TLABEL>& state)
        : m_transitions(state.m_transitions), m_label(state.m_label)
    {
    }
    virtual ~NFABaseState()
    {
    }
    void add_transition(const NFATransition<TCHAR>& transition)
    {
        m_transitions.push_back(transition);
    }
    void add_transition(const NFACharClass<TCHAR>& cc, int dest)
    {
        m_transitions.emplace_back(cc, dest);
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
    NFABaseState<TCHAR, TLABEL> offset(int value) const
    {
        NFABaseState<TCHAR, TLABEL> new_state;

        for (const auto& transition : m_transitions)
        {
            new_state.add_transition(transition.offset(value));
        }

        return new_state;
    }
    void rebase_transitions(const NFABaseState<TCHAR, TLABEL>& src, int offset_value)
    {
        for (const auto& i : src.m_transitions)
        {
            add_transition(i.offset(offset_value));
        }
    }
    IndexVector epsilon_transitions() const
    {
        IndexVector ret;

        for (const auto& i : m_transitions)
        {
            if (i.is_epsilon())
            {
                ret.push_back(i.dest());
            }
        }

        return ret;
    }
    const std::vector<NFATransition<TCHAR> >& get_transitions() const
    {
        return m_transitions;
    }
};

template<typename TCHAR> using NFAState = NFABaseState<TCHAR, int>;

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

        auto i = nfa.m_states.cbegin() + 1;

        for (; i != nfa.m_states.cend(); i++)
        {
            //Copy all states except the initial state
            m_states.push_back(i->offset(initial_size - 1));
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

        auto i = nfa.m_states.cbegin() + 1;

        for (; i != nfa.m_states.cend(); i++)
        {
            //Copy all states except the initial state
            m_states.push_back(i->offset(initial_size - 1));
        }

        //Rebase the transitions from the start state to the newly added states
        m_states[0].rebase_transitions(nfa.m_states[0], initial_size - 1);

        //Add the join state
        add_state(NFACharClass<TCHAR>());

        add_transition_from(NFACharClass<TCHAR>(), initial_size - 1);
    }
    NFA()
    {
        m_states.emplace_back();
    }
    virtual ~NFA()
    {
    }
    /*!
     * @brief Add a transition to a new state from the final state.
     */
    void add_state(const NFACharClass<TCHAR>& cc)
    {
        m_states.back().add_transition(cc, m_states.size());
        m_states.emplace_back();
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
    void print_graph(std::ostream& of, const std::string& graph_name)
    {
        of << "digraph " << graph_name << "{" << std::endl;

        of << "graph [ charset=\"UTF-8\", style=\"filled\" ];" << std::endl;

        of << "node [ style=\"solid,filled\" ];" << std::endl;

        of << "edge [ style=\"solid\" ];" << std::endl;

        for (unsigned int i = 0; i < m_states.size(); i++)
        {
            of << "S" << i << " [shape = circle];" << std::endl;
        }

        for (unsigned int i = 0; i < m_states.size(); i++)
        {
            m_states[i].print_transitions(of, i);
        }

        of << "}" << std::endl;
    }
    IndexVector epsilon_closure_sub(int index)
    {
        IndexVector ret;

        //Always include myself
        ret.push_back(index);

        IndexVector etr = m_states[index].epsilon_transitions();

        for (int dest : etr)
        {
            ret += epsilon_closure_sub(dest);
        }

        return ret;
    }
    /*!
     * @brief Collect the epsilon closure around the state
     */
    IndexVector epsilon_closure(int index)
    {
        IndexVector ret = epsilon_closure_sub(index);

        return ret.sort_and_unique_copy();
    }
    IndexVector epsilon_closure(const IndexVector& indices)
    {
        IndexVector ret;

        for (int index : indices)
        {
            ret += epsilon_closure_sub(index);
        }

        return ret.sort_and_unique_copy();
    }
    /*!
     * @brief Collect all the transitions from the closure
     */
    std::vector<NFATransition<TCHAR>&> gather_transitions(const std::vector<int>& closure)
    {
        std::vector<NFATransition<TCHAR>&> ret;
        for (int index : closure)
        {
            const std::vector<NFATransition<TCHAR> >& tr;

            ret.insert(ret.end(), tr.cbegin(), tr.cend());
        }
        return ret;
    }
};
}
