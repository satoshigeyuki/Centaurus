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
template<> struct hash<std::pair<Centaurus::ATNPath, int> >
{
    size_t operator()(const std::pair<Centaurus::ATNPath, int>& p) const
    {
        return p.first.hash() + p.second;
    }
};
template<> struct equal_to<std::pair<Centaurus::ATNPath, int> >
{
    bool operator()(const std::pair<Centaurus::ATNPath, int>& x, const std::pair<Centaurus::ATNPath, int>& y) const
    {
        return x.first == y.first && x.second == y.second;
    }
};
}

namespace Centaurus
{
template<typename TCHAR> using LDFATransition = NFATransition<TCHAR>;

template<typename TCHAR>
class LDFAState : public NFABaseState<TCHAR, CATNClosure>
{
    using NFABaseState<TCHAR, CATNClosure>::m_label;
    using NFABaseState<TCHAR, CATNClosure>::m_transitions;

public:
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
    LDFAState()
    {
        //State color is set to WHITE
    }
    LDFAState(const CATNClosure& label)
        : NFABaseState<TCHAR, CATNClosure>(label)
    {
    }
    virtual ~LDFAState()
    {
    }
};

template<typename TCHAR>
class LDFAEquivalenceTable : public std::vector<std::pair<CharClass<TCHAR>, CATNClosure> >
{
public:
    LDFAEquivalenceTable()
    {
    }
    virtual ~LDFAEquivalenceTable()
    {
    }
    void add_transition(const Range<TCHAR>& r, const CATNClosure& dests) const
    {
        for (auto& item : *this)
        {
            if (std::equal(item.second.cbegin(), item.second.cend(), dests.cbegin()))
            {
                item.first |= r;
                return;
            }
        }
        emplace_back(CharClass<TCHAR>(r), dests);
    }
};

template<typename TCHAR>
class LookaheadDFA : public NFABase<TCHAR>
{
    std::vector<LDFAState<TCHAR> > m_states;
private:
    int add_state(const CATNClosure& label)
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

        //Collect all outbound (non-epsilon) transitions
        // originating from the closure associated with the current LDFA state.
        for (const std::pair<ATNPath, int>& p : m_states[index].label())
        {
            for (const auto& tr : catn.get_transitions(p.first))
            {
                if (!tr.is_epsilon())
                {
                    //Mark the outbound transition with the color of the origin node
                    outbound_transitions.emplace_back(p.second, tr);

                    /*if (catn.get_node(tr.dest()).get_type() == CATNNodeType::Barrier)
                    {
                        throw SimpleException("Barrier node reached during LDFA construction.");
                    }*/
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
            CATNClosure dests;

            for (const auto& p : outbound_transitions)
            {
                if (p.second.label().includes(r))
                {
                    dests.emplace(p.second.dest(), p.first);
                }
            }

            if (!dests.empty())
            {
                table.add_transition(r, dests);
            }
        }

        //Add new states to the LDFA
        int initial_index = m_states.size();

        for (const auto& item : table)
        {
            CATNClosure ec = catn.build_closure(item.second);

            int new_index = add_state(ec);

            m_states[index].add_transition(item.first, new_index);
        }

        //Recursively construct the closures for all the newly added DFA states
        for (int i = initial_index; i < m_states.size(); i++)
        {
            fork_closures(catn, i);
        }
    }
public:
    LookaheadDFA(const CompositeATN<TCHAR>& catn, const ATNPath& origin)
    {
        m_states.emplace_back(catn.build_root_closure(origin));

        fork_closures(catn, 0);
    }
    virtual ~LookaheadDFA()
    {
    }
};
}
