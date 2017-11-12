#pragma once

#include <vector>
#include <set>
#include <utility>

#include "catn.hpp"
#include "dfa.hpp"
#include "nfa.hpp"
#include "util.hpp"

namespace std
{
template<> struct hash<std::pair<int, int> >
{
    size_t operator()(const std::pair<int, int>& p) const
    {
        return p.first + p.second;
    }
};
template<> struct equal_to<std::pair<int, int> >
{
    bool operator()(const std::pair<int, int>& x, const std::pair<int, int>& y) const
    {
        return x.first == y.first && x.second == y.second;
    }
};
}

namespace Centaurus
{
template<typename TCHAR> using LDFATransition = NFATransition<TCHAR>;
using LDFAStateLabel = std::set<std::pair<int, int> >;
template<typename TCHAR> using LDFAState = NFABaseState<TCHAR, LDFAStateLabel>;
template<typename TCHAR>
class LookaheadDFA
{
    using LDFAEquivalenceTable = std::vector<std::pair<CharClass<TCHAR>, LDFAStateLabel> >;

    std::vector<LDFAState<TCHAR> > m_states;
private:
    void fork_closures(const CompositeATN<TCHAR>& catn, int index)
    {
        //States reached from the current LDFA state (epsilon closure) via non-epsilon transitions
        std::vector<std::pair<int, CATNTransition<TCHAR> > > outbound_transitions;

        //Collect all outbound transitions
        for (std::pair<int, int> p : m_states[index].label())
        {
            for (const auto& tr : catn.get_transitions(p.first))
            {
                if (!tr.is_epsilon())
                {
                    outbound_transitions.emplace_back(p.second, tr);
                }
            }
        }

        //Divide the entire character space into equivalent sets
        std::set<int> borders;
        for (const auto& p : outbound_transitions)
        {
            IndexVector mb = p.second.label().collect_borders();

            borders.insert(mb.cbegin(), mb.cend());
        }

        LDFAEquivalenceTable table;

        //Contract equivalent transitions using the equivalence table
        for (auto i = borders.cbegin(); i != borders.cend(); i++)
        {
            //Iterate over all equivalent ranges
            TCHAR range_start = *i;
            if (++i == borders.cend()) break;
            TCHAR range_end = *i;

            Range<TCHAR> r(range_start, range_end);
            std::set<std::pair<int, int> > dests;

            for (const auto& p : outbound_transitions)
            {
                if (p.second.label().includes(r))
                {
                    dests.emplace(tr.dest(), p.first);
                }
            }

            if (!dests.empty())
            {
                add_transition(table, r, dests);
            }
        }

        //Add new states to the LDFA
        int initial_index = m_states.size();

        for (const auto& item : table)
        {
            LDFAStateLabel ec = build_closure(catn, item.second);

            int new_index = add_state(ec);

            m_states[index].add_transition(item.first, new_index);
        }

        for (int i = initial_index; i < m_states.size(); i++)
        {
            fork_closures(catn, i);
        }
    }
    void build_closure_sub(const CompositeATN<TCHAR>& catn, LDFAStateLabel& label, int origin, int color)
    {
        label.emplace(origin, color);

        for (const auto& tr : catn.get_transitions(origin))
        {
            if (tr.is_epsilon())
            {
                build_closure_sub(catn, label, tr.dest(), color);
            }
        }
    }
    LDFAStateLabel build_closure(const CompositeATN<TCHAR>& catn, const LDFAStateLabel& src)
    {
        LDFAStateLabel label;

        for (std::set<int, int> p : src)
        {
            build_closure_sub(catn, label, p.first, p.second);
        }

        return label;
    }
    LDFAStateLabel build_root_closure(const CompositeATN<TCHAR>& catn, int origin)
    {
        LDFAStateLabel label;

        label.emplace(origin, WHITE);

        const std::vector<CATNTransition<TCHAR> >& transitions = catn.get_transitions(origin);

        for (unsigned int i = 0; i < transitions.size(); i++)
        {
            //All transitions from the decision point must be epsilon transitions
            assert(transitions[i].is_epsilon());

            build_closure_sub(catn, label, transitions[i].dest(), i + 1);
        }

        return label;
    }
public:
    LookaheadDFA(const CompositeATN<TCHAR>& catn, int origin)
    {
    }
    virtual ~LookaheadDFA()
    {
    }
};
}
