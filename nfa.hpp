#pragma once

#include <vector>
#include <set>
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
#include "charclass.hpp"

namespace Centaur
{
template<typename TCHAR> class NFATransition
{
    CharClass<TCHAR> m_label;
    int m_dest;
public:
    NFATransition(const CharClass<TCHAR>& label, int dest)
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
    const CharClass<TCHAR>& label() const
    {
        return m_label;
    }
    int dest() const
    {
        return m_dest;
    }
    void add_class(const CharClass<TCHAR>& cc)
    {
        m_label |= cc;
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
    void add_transition(const CharClass<TCHAR>& cc, int dest)
    {
        if (!cc.is_epsilon())
        {
            for (auto& tr : m_transitions)
            {
                if (tr.dest() == dest && !tr.label().is_epsilon())
                {
                    tr.add_class(cc);
                    return;
                }
            }
        }
        m_transitions.emplace_back(cc, dest);
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
    void print(std::ostream& os, int from) const
    {
        for (const auto& t : m_transitions)
        {
            os << "S" << from << " -> " << "S" << t.dest() << " [ label=\"";
            os << t.label();
            os << "\" ];" << std::endl;
        }
    }
    const TLABEL& label() const
    {
        return m_label;
    }
};
template<typename TCHAR> using NFAState = NFABaseState<TCHAR, int>;

template<typename TSTATE> class NFABase
{
protected:
    std::vector<TSTATE> m_states;
public:
    NFABase()
    {
    }
    virtual ~NFABase()
    {
    }
    virtual void print_state(std::ostream& os, int index)
    {
        os << index;
    }
    void print(std::ostream& os, const std::string& graph_name)
    {
        os << "digraph " << graph_name << " {" << std::endl;

        os << "graph [ charset=\"UTF-8\", style=\"filled\" ];" << std::endl;

        os << "node [ style=\"solid,filled\" ];" << std::endl;

        os << "edge [ style=\"solid\" ];" << std::endl;

        for (unsigned int i = 0; i < m_states.size(); i++)
        {
            os << "S" << i << " [ label=\"";
            print_state(os, i);
            os << "\", shape=circle ];" << std::endl;
        }

        for (unsigned int i = 0; i < m_states.size(); i++)
        {
            m_states[i].print(os, i);
        }

        os << "}" << std::endl;
    }
};

template<typename TCHAR> class NFA : public NFABase<NFAState<TCHAR> >
{
    using NFABase<NFAState<TCHAR> >::m_states;
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
            m_states.push_back(i->offset(initial_size - 1));
        }

        m_states[initial_size - 1].rebase_transitions(nfa.m_states[0], initial_size - 1);
    }
    /*!
     * @brief Form a selection with another NFA
     */
    void select(const NFA<TCHAR>& nfa)
    {
        int initial_size = m_states.size();

        m_states[0].add_transition(CharClass<TCHAR>(), initial_size);

        auto i = nfa.m_states.cbegin() + 1;

        for (; i != nfa.m_states.cend(); i++)
        {
            m_states.push_back(i->offset(initial_size - 1));
        }

        m_states[0].rebase_transitions(nfa.m_states[0], initial_size - 1);

        //Add the join state
        add_state(CharClass<TCHAR>());

        add_transition_from(CharClass<TCHAR>(), initial_size - 1);
    }
    NFA()
    {
        m_states.emplace_back();
        add_state(CharClass<TCHAR>());
    }
    virtual ~NFA()
    {
    }
    /*!
     * @brief Add a transition to a new state from the final state.
     */
    void add_state(const CharClass<TCHAR>& cc)
    {
        m_states.back().add_transition(cc, m_states.size());
        m_states.emplace_back();
    }
    /*!
     * @brief Add a transition from the final state to another state.
     */
    void add_transition_to(const CharClass<TCHAR>& cc, int dest)
    {
        m_states.back().add_transition(cc, dest);
    }
    /*!
     * @brief Add a transition to the final state from another state.
     */
    void add_transition_from(const CharClass<TCHAR>& cc, int src)
    {
        m_states[src].add_transition(cc, m_states.size() - 1);
    }
    void epsilon_closure_sub(std::set<int>& closure, int index) const
    {
        //Always include myself
        closure.insert(index);

        for (int dest : m_states[index].epsilon_transitions())
        {
            epsilon_closure_sub(closure, dest);
        }
    }
    /*!
     * @brief Collect the epsilon closure around the state
     */
    std::set<int> epsilon_closure(int index) const
    {
        std::set<int> ret;

        epsilon_closure_sub(ret, index);

        return ret;
    }
    std::set<int> epsilon_closure(const std::set<int>& indices) const
    {
        std::set<int> ret;

        for (int index : indices)
        {
            epsilon_closure_sub(ret, index);
        }

        return ret;
    }
    const std::vector<NFATransition<TCHAR> >& get_transitions(int index) const
    {
        return m_states[index].get_transitions();
    }
};
}
