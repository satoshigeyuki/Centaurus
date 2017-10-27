#pragma once

#include <utility>
#include <set>
#include <algorithm>
#include <vector>

#include "nfa.hpp"

namespace Centaur
{
template<typename TCHAR> using DFATransition = NFATransition<TCHAR>;
template<typename TCHAR> using DFAState = NFABaseState<TCHAR, IndexVector>;

template<typename TCHAR> class DFA : public NFABase<DFAState<TCHAR> >
{
    using NFABase<DFAState<TCHAR> >::m_states;
    using EquivalenceTable = std::vector<std::pair<CharClass<TCHAR>, std::set<int> > >;

    /*!
     * @brief Add a transition to EquivalenceTable
     */
    void add_transition(EquivalenceTable& table, const Range<TCHAR>& r, const std::set<int>& dests) const
    {
        for (auto& item : table)
        {
            if (std::equal(item.second.cbegin(), item.second.cend(), dests.cbegin()))
            {
                item.first |= r;
                return;
            }
        }
        table.emplace_back(CharClass<TCHAR>(r), dests);
    }
    /*!
     * @brief Create a state with a label if it does not exist
     */
    int add_state(const std::set<int>& label)
    {
        for (unsigned int i = 0; i < m_states.size(); i++)
        {
            if (std::equal(label.cbegin(), label.cend(), m_states[i].label().cbegin()))
            {
                return i;
            }
        }
        m_states.emplace_back(IndexVector(label.cbegin(), label.cend()));
        return m_states.size() - 1;
    }
    void fork_states(int index, const NFA<TCHAR>& nfa)
    {
        //States reached via non-epsilon transitions
        std::vector<NFATransition<TCHAR> > noneps_tr;

        for (int i : m_states[index].label())
        {
            for (const auto& tr : nfa.get_transitions(i))
            {
                if (!tr.is_epsilon())
                {
                    noneps_tr.push_back(tr);
                }
            }
        }

        //All borders included in the transition set
        std::set<int> borders;

        for (const auto& tr : noneps_tr)
        {
            IndexVector mb = tr.label().collect_borders();

            borders.insert(mb.cbegin(), mb.cend());
        }

        EquivalenceTable table;

        for (auto i = borders.cbegin(); i != borders.cend(); )
        {
            TCHAR range_start = *i;
            if (++i == borders.cend()) break;
            TCHAR range_end = *i;

            Range<TCHAR> r(range_start, range_end);

            std::set<int> dests;

            for (const auto& tr : noneps_tr)
            {
                if (tr.label().includes(r))
                {
                    dests.insert(tr.dest());
                }
            }

            add_transition(table, r, dests);
        }

        int initial_index = m_states.size();
        for (const auto& item : table)
        {
            std::set<int> ec = nfa.epsilon_closure(item.second);

            int new_index = add_state(ec);

            m_states[index].add_transition(item.first, new_index);
        }

        for (int i = initial_index; i < m_states.size(); i++)
        {
            fork_states(i, nfa);
        }
    }
public:
    DFA(const NFA<TCHAR>& nfa)
    {
        //Start by collecting the epsilon closure of start state
        std::set<int> ec0 = nfa.epsilon_closure(0);

        //Create the start state
        m_states.emplace_back(IndexVector(ec0.cbegin(), ec0.cend()));

        //Recursively construct the DFA
        fork_states(0, nfa);
    }
    DFA()
    {
    }
    virtual ~DFA()
    {
    }
};
}
