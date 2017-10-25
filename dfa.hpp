#pragma once

#include <vector>
#include <limits>
#include <assert.h>

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

        m_start = wide_to_target(start);
        m_end = wide_to_target(end);
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
            if (r.end() == r.start() + 1)
            {
                if (r.start() == wide_to_target(L'"'))
                    os << "\\\"";
                else
                    os << r.start();
            }
            else
            {
                if (r.start() == wide_to_target(L'"'))
                    os << "\\\"";
                else
                    os << r.start();
                os << '-';
                if (r.end() == wide_to_target(L'"'))
                    os << "\\\"";
                else
                    os << r.end();
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

template<typename TCHAR> class NFAState
{
    std::vector<NFATransition<TCHAR> > m_transitions;
    NFA& m_parent;
public:
    NFAState(NFA& parent)
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
    void print_transitions(std::ostream& of, int from) const
    {
        for (const auto& t : m_transitions)
        {
            os << "S" << from << " -> " << "S" << t.dest() << " [ label=\"";
            t.label().print(of);
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
};

template<typename TCHAR> class NFA
{
    std::vector<NFAState<TCHAR> > m_states;
    int m_final_state;
    int append(const NFA<TCHAR>& nfa)
    {
        int initial_size = m_states.size();
        for (const auto& state : nfa.m_states)
        {
            m_states.push_back(state.offset(*this, initial_size));
        }
        return initial_size;
    }
public:
    NFA()
    {
        m_states.emplace_back(*this);
        m_final_state = 0;
    }
    virtual ~NFA()
    {
    }
    /*!
     * @brief Add a transition to a new state from the final state.
     */
    int add_state(const NFACharClass<TCHAR>& cc)
    {
        int ret = m_final_state;
        m_states.emplace_back();
        m_states[m_final_state].add_transition(cc, m_states.size() - 1);
        m_final_state = m_states.size() - 1;
        return ret;
    }
    /*!
     * @brief Concatenate another NFA to the final state
     */
    int concat(const NFA<TCHAR>& nfa)
    {
        int start = append(nfa);
        add_transition_to(NFACharClass<TCHAR>(), start);
        m_final_state = m_states.size() - 1;
        return start;
    }
    /*!
     * @brief Form a selection with another NFA
     */
    int select(const NFA<TCHAR>& nfa, int from)
    {
        int start = append(nfa);
        int end = m_states.size() - 1;
        add_transition_from_to(NFACharClass<TCHAR>(), from, start);
        add_state(NFACharClass<TCHAR>());
        add_transition_from(NFACharClass<TCHAR>(), end);
        return from;
    }
    /*!
     * @brief Add a transition from the final state to another state.
     */
    void add_transition_to(const NFACharClass<TCHAR>& cc, int dest)
    {
        m_states[m_final_state].add_transition(cc, dest);
    }
    /*!
     * @brief Add a transition to the final state from another state.
     */
    void add_transition_from(const NFACharClass<TCHAR>& cc, int src)
    {
        m_states[src].add_transition(cc, m_final_state);
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
