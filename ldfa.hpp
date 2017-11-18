#pragma once

#include <iostream>
#include <vector>
#include <set>
#include <utility>

#include "catn2.hpp"
#include "nfa.hpp"
#include "util.hpp"
#include "exception.hpp"

namespace std
{
template<> struct hash<std::pair<ATNPath, int> >
{
    size_t operator()(const std::pair<ATNPath, int>& p) const
    {
        return p.first.hash() + p.second;
    }
};
template<> struct equal_to<std::pair<ATNPath, int> >
{
    bool operator()(const std::pair<ATNPath, int>& x, const std::pair<ATNPath, int>& y) const
    {
        return x.first == y.first && x.second == y.second;
    }
};
}

namespace Centaurus
{
template<typename TCHAR> using LDFATransition = NFATransition<TCHAR>;
using LDFAStateLabel = std::set<std::pair<ATNPath, int> >;
template<typename TCHAR>
class LDFAState : public NFABaseState<TCHAR, LDFAStateLabel>
{
    using NFABaseState<TCHAR, LDFAStateLabel>::m_label;
    using NFABaseState<TCHAR, LDFAStateLabel>::m_transitions;

private:
    int get_color() const
    {
        int color = 0;
        for (const auto& p : m_label)
        {
            if (color == 0)
            {
                color = p.second;
            }
            else
            {
                if (color != p.second && p.second != 0)
                    return -1;
            }
        }
        return color;
    }
public:
    LDFAState()
    {
        //State color is set to WHITE
    }
    LDFAState(const LDFAStateLabel& label)
        : NFABaseState<TCHAR, LDFAStateLabel>(label)
    {
    }
    virtual ~LDFAState()
    {
    }
};
template<typename TCHAR>
class LookaheadDFA : public NFABase<TCHAR>
{
    using LDFAEquivalenceTable = std::vector<std::pair<CharClass<TCHAR>, LDFAStateLabel> >;

    std::vector<LDFAState<TCHAR> > m_states;
private:
    void add_transition(LDFAEquivalenceTable& table, const Range<TCHAR>& r, const LDFAStateLabel& dests) const
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
    int add_state(const LDFAStateLabel& label)
    {
        for (unsigned int i = 0; i < m_states.size(); i++)
        {
            if (std::equal(label.cbegin(), label.cend(), m_states[i].label().cbegin()))
            {
                return i;
            }
        }
        m_states.emplace_back(label);
        return m_states.size() - 1;
    }
    void fork_closures(const CompositeATN<TCHAR>& catn, int index)
    {
        //First, check if the state in question is already single-colored
        if (m_states[index].get_color() > 0)
        {
            return;
        }

        //States reached from the current LDFA state (epsilon closure) via non-epsilon transitions
        std::vector<std::pair<int, CATNTransition<TCHAR> > > outbound_transitions;

        //Collect all outbound transitions
        for (std::pair<ATNPath, int> p : m_states[index].label())
        {
            for (const auto& tr : catn.get_transitions(p.first))
            {
                if (!tr.is_epsilon())
                {
                    //Mark the outbound transition with the color of the origin node
                    outbound_transitions.emplace_back(p.second, tr);

                    if (catn.get_node(tr.dest()).get_type() == CATNNodeType::Barrier)
                    {
                        throw SimpleException("Barrier node reached during LDFA construction.");
                    }
                }
            }
        }

        int color = m_states[index].get_color();

        if (outbound_transitions.empty() && color < 0)
        {
            //If the color is BLACK and there is no transitions outbound,
            //we throw an exception because the decision is impossible
            throw SimpleException("LDFA construction failed.");
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
        for (auto i = borders.cbegin(); i != borders.cend(); )
        {
            //Iterate over all equivalent ranges
            TCHAR range_start = *i;
            if (++i == borders.cend()) break;
            TCHAR range_end = *i;

            Range<TCHAR> r(range_start, range_end);

            //Label of the new destination LDFA state,
            //which is equivalent to the set of CATN nodes reached from 
            //the closure, marked with their respective colors
            LDFAStateLabel dests;

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

        for (std::pair<int, int> p : src)
        {
            build_closure_sub(catn, label, p.first, p.second);
        }

        return label;
    }
    LDFAStateLabel build_root_closure(const CompositeATN<TCHAR>& catn, int origin)
    {
        LDFAStateLabel label;

        label.emplace(origin, 0);

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
        m_states.emplace_back(build_root_closure(catn, origin));

        fork_closures(catn, 0);
    }
    virtual ~LookaheadDFA()
    {
    }
    virtual void print_state(std::ostream& os, int index)
    {
        os << m_states[index].label();
    }
};
}
