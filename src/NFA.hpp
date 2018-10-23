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

#include "Util.hpp"
#include "Exception.hpp"
#include "CharClass.hpp"

namespace Centaurus
{
template<typename TCHAR> class NFATransition
{
    CharClass<TCHAR> m_label;
    int m_dest;
    bool m_long;
public:
    NFATransition(const CharClass<TCHAR>& label, int dest, bool long_flag = false)
        : m_label(label), m_dest(dest), m_long(long_flag)
    {
    }
    NFATransition(const NFATransition<TCHAR>& transition)
        : m_label(transition.m_label), m_dest(transition.m_dest), m_long(transition.m_long)
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
    void dest(int new_dest)
    {
        m_dest = new_dest;
    }
    void add_class(const CharClass<TCHAR>& cc)
    {
        m_label |= cc;
    }
    NFATransition<TCHAR> offset(int value) const
    {
        return NFATransition<TCHAR>(m_label, m_dest + value, m_long);
    }
    bool is_epsilon() const
    {
        return m_label.is_epsilon();
    }
    bool operator<(const NFATransition<TCHAR>& t) const
    {
        return m_dest < t.m_dest;
    }
    bool operator==(const NFATransition<TCHAR>& t) const
    {
        return m_dest == t.m_dest && m_label == t.m_label;
    }
    size_t hash() const
    {
        std::hash<int> int_hasher;
        return int_hasher(m_dest) ^ m_label.hash();
    }
    bool is_long() const
    {
        return m_long;
    }
    void set_long(bool long_flag = true)
    {
        m_long = long_flag;
    }
};

template<typename TCHAR> class NFA;

template<typename TCHAR, typename TLABEL> class NFABaseState
{
protected:
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
    NFABaseState(const NFABaseState<TCHAR, TLABEL>& state, std::vector<NFATransition<TCHAR> >&& transitions)
        : m_transitions(transitions), m_label(state.m_label)
    {
    }
    virtual ~NFABaseState()
    {
    }
    void add_transition(const NFATransition<TCHAR>& transition)
    {
        m_transitions.push_back(transition);
    }
    void add_transition(const CharClass<TCHAR>& cc, int dest, bool long_flag = false)
    {
        if (!cc.is_epsilon())
        {
            for (auto& tr : m_transitions)
            {
                if (tr.dest() == dest && !tr.label().is_epsilon())
                {
                    tr.add_class(cc);
                    if (long_flag) tr.set_long();
                    return;
                }
            }
        }
        m_transitions.emplace_back(cc, dest, long_flag);
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
    IndexVector epsilon_transitions(bool& long_flag) const
    {
        IndexVector ret;

        for (const auto& i : m_transitions)
        {
            if (i.is_epsilon())
            {
                ret.push_back(i.dest());
            }
            if (i.is_long())
            {
                long_flag = true;
            }
        }

        return ret;
    }
    const std::vector<NFATransition<TCHAR> >& get_transitions() const
    {
        return m_transitions;
    }
    std::vector<NFATransition<TCHAR> >& get_transitions()
    {
        return m_transitions;
    }
	virtual void print(std::wostream& os, int from) const
	{
		for (const auto& t : m_transitions)
		{
			os << L"S" << from << L" -> " << L"S" << t.dest() << L" [ label=\"";
			os << t.label();
            if (!t.is_long())
            {
                os << L"\" ];" << std::endl;
            }
            else
            {
                os << L"\", penwidth=3, arrowsize=3 ];" << std::endl;
            }
		}
	}
	void print_subgraph(std::wostream& os, int from, const std::wstring& prefix) const
	{
		for (const auto& t : m_transitions)
		{
			os << prefix << L"_S" << from << L" -> " << prefix << L"_S" << t.dest() << L" [ label=\"";
			os << t.label();
            if (!t.is_long())
            {
                os << L"\" ];" << std::endl;
            }
            else
            {
                os << L"\", penwidth=3, arrowsize=3 ];" << std::endl;
            }
		}
	}
    const TLABEL& label() const
    {
        return m_label;
    }
    bool operator==(const NFABaseState<TCHAR, TLABEL>& s) const
    {
		if (m_transitions.size() != s.m_transitions.size()) return false;
        return std::equal(m_transitions.cbegin(), m_transitions.cend(), s.m_transitions.cbegin());
    }
    size_t hash() const
    {
        size_t value = 0;
        for (const auto& t : m_transitions)
        {
            value ^= t.hash();
        }
        return value;
    }
    void sort()
    {
        std::sort(m_transitions.begin(), m_transitions.end());
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
    NFABase(NFABase<TSTATE>&& old)
        : m_states(std::move(old.m_states))
    {
    }
    NFABase(const NFABase<TSTATE>& old)
        : m_states(old.m_states)
    {
    }
    virtual ~NFABase()
    {
    }
	virtual void print_state(std::wostream& os, int index) const
	{
		os << index;
	}
	virtual void print(std::wostream& os, const std::wstring& graph_name) const
	{
		os << L"digraph " << graph_name << L" {" << std::endl;
		os << L"rankdir=\"LR\";" << std::endl;
		os << L"graph [ charset=\"UTF-8\", style=\"filled\" ];" << std::endl;
		os << L"node [ style=\"solid,filled\" ];" << std::endl;
		os << L"edge [ style=\"solid\" ];" << std::endl;

		for (unsigned int i = 0; i < m_states.size(); i++)
		{
			os << L"S" << i << L" [ label=\"";
			print_state(os, i);
			os << L"\", shape=circle ];" << std::endl;
		}

		for (unsigned int i = 0; i < m_states.size(); i++)
		{
			m_states[i].print(os, i);
		}

		os << L"}" << std::endl;
	}
	void print_subgraph(std::wostream& os, int index, const std::wstring& prefix) const
	{
		os << L"subgraph cluster_" << prefix << L" {" << std::endl;
		os << L"label=\"[" << index << L"]\"" << std::endl;
		for (unsigned int i = 0; i < m_states.size(); i++)
		{
			os << prefix << L"_S" << i << L" [ label=\"";
			print_state(os, i);
			os << L"\", shape=circle ];" << std::endl;
		}
		for (unsigned int i = 0; i < m_states.size(); i++)
		{
			m_states[i].print_subgraph(os, i, prefix);
		}
		os << L"}" << std::endl;
	}
	std::wstring get_entry_wide(const std::wstring& prefix) const
	{
		return prefix + L"_S0";
	}
	std::wstring get_exit_wide(const std::wstring& prefix) const
	{
		return prefix + L"_S" + std::to_wstring(m_states.size() - 1);
	}
    int get_state_num() const
    {
        return m_states.size();
    }
    const TSTATE& get_state(int index) const
    {
        return m_states[index];
    }
};

using NFAClosure = std::set<int>;

template<typename TCHAR>
class NFADepartureSet : public std::vector<std::pair<CharClass<TCHAR>, NFAClosure> >
{
public:
    NFADepartureSet()
    {
    }
    virtual ~NFADepartureSet()
    {
    }
    void add(const Range<TCHAR>& r, const NFAClosure& closure)
    {
        for (auto& p : *this)
        {
            if (std::equal(closure.cbegin(), closure.cend(), p.second.cbegin()))
            {
                p.first |= r;
                return;
            }
        }
        this->emplace_back(CharClass<TCHAR>(r), closure);
    }
};

template<typename TCHAR>
class NFADepartureSetFactory
{
    std::vector<NFATransition<TCHAR> > m_transitions;
public:
    NFADepartureSetFactory()
    {

    }
    virtual ~NFADepartureSetFactory()
    {

    }
    void add(const NFATransition<TCHAR>& tr)
    {
        if (!tr.is_epsilon())
            m_transitions.push_back(tr);
    }
    NFADepartureSet<TCHAR> build_departure_set()
    {
        std::set<int> borders;

        for (const auto& tr : m_transitions)
        {
            IndexVector borders_for_one_tr = tr.label().collect_borders();

            borders.insert(borders_for_one_tr.cbegin(), borders_for_one_tr.cend());
        }

        NFADepartureSet<TCHAR> deptset;

        for (auto i = borders.cbegin(); i != borders.cend();)
        {
            TCHAR atomic_range_start = *i;
            if (++i == borders.cend()) break;
            TCHAR atomic_range_end = *i;

            Range<TCHAR> atomic_range(atomic_range_start, atomic_range_end);

            NFAClosure closure;

            for (const auto& tr : m_transitions)
            {
                if (tr.label().includes(atomic_range))
                {
                    closure.insert(tr.dest());
                }
            }

            if (!closure.empty())
            {
                deptset.add(atomic_range, closure);
            }
        }

        return deptset;
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
    void parse(Stream& stream)
    {
        for (wchar_t ch = stream.get(); ch != L')' && ch != L'/' && ch != L'\0'; ch = stream.get())
        {
            NFA<TCHAR> nfa;
            nfa.parse_selection(stream, ch);
            concat(nfa);
        }
    }
    void parse_unit(Stream& stream, wchar_t ch)
    {
        switch (ch)
        {
        case L'\\':
            ch = stream.get();
            switch (ch)
            {
            case L'[':
            case L']':
            case L'+':
            case L'*':
            case L'{':
            case L'}':
            case L'?':
            case L'\\':
            case L'|':
            case L'(':
            case L')':
            case L'.':
                add_state(CharClass<TCHAR>(ch));
                break;
            default:
                throw stream.unexpected(ch);
            }
            break;
        case L'[':
            add_state(CharClass<TCHAR>(stream));
            break;
        case L'.':
            add_state(CharClass<TCHAR>::make_star());
            break;
        case L'(':
            concat(NFA<TCHAR>(stream));
            break;
        default:
            add_state(CharClass<TCHAR>(ch));
            break;
        }
        ch = stream.peek();
        switch (ch)
        {
        case L'+':
            stream.discard();
            ch = stream.peek();
            switch (ch)
            {
            case L'+':
                stream.discard();
                //Add an epsilon transition to the last state
                add_transition_to(CharClass<TCHAR>(), 1, true);
                break;
            case L'?':
                stream.discard();
                //Add an epsilon transition to the last state
                add_transition_to(CharClass<TCHAR>(), 1);
                break;
            case L'*':
                stream.discard();
                //Add an epsilon transition to the last state
                add_transition_to(CharClass<TCHAR>(), 1);
                break;
            default:
                //Add an epsilon transition to the last state
                add_transition_to(CharClass<TCHAR>(), 1);
                break;
            }
            break;
        case L'*':
            stream.discard();
            ch = stream.peek();
            switch (ch)
            {
            case L'+':
                stream.discard();
                //Add an epsilon transition to the last state
                add_transition_to(CharClass<TCHAR>(), 1, true);
                //Add a confluence
                add_state(CharClass<TCHAR>());
                //Add a skipping transition
                add_transition_from(CharClass<TCHAR>(), 1);
                break;
            case L'?':
                stream.discard();
                //Add an epsilon transition to the last state
                add_transition_to(CharClass<TCHAR>(), 1);
                //Add a confluence
                add_state(CharClass<TCHAR>());
                //Add a skipping transition
                add_transition_from(CharClass<TCHAR>(), 1);
                break;
            case L'*':
                stream.discard();
                //Add an epsilon transition to the last state
                add_transition_to(CharClass<TCHAR>(), 1);
                //Add a confluence
                add_state(CharClass<TCHAR>());
                //Add a skipping transition
                add_transition_from(CharClass<TCHAR>(), 1);
                break;
            default:
                //Add an epsilon transition to the last state
                add_transition_to(CharClass<TCHAR>(), 1);
                //Add a confluence
                add_state(CharClass<TCHAR>());
                //Add a skipping transition
                add_transition_from(CharClass<TCHAR>(), 1);
                break;
            }
            break;
        case L'?':
            stream.discard();
            ch = stream.peek();
            switch (ch)
            {
            case L'?':
                stream.discard();
                //Add a skipping transition
                add_transition_from(CharClass<TCHAR>(), 1);
                break;
            case L'*':
                stream.discard();
                //Add a skipping transition
                add_transition_from(CharClass<TCHAR>(), 1);
                break;
            default:
                //Add a skipping transition
                add_transition_from(CharClass<TCHAR>(), 1);
                break;
            }
            break;
        }
    }
    void parse_selection(Stream& stream, wchar_t ch)
    {
        parse_unit(stream, ch);

        while (true)
        {
            ch = stream.peek();

            if (ch != L'|')
            {
                break;
            }

            stream.discard();

            NFA<TCHAR> nfa;
            nfa.parse_unit(stream, stream.get());
            select(nfa);
        }
    }
    NFA(Stream& stream)
    {
        m_states.emplace_back();
        add_state(CharClass<TCHAR>());
        
        parse(stream);
    }
    NFA()
    {
        m_states.emplace_back();
        add_state(CharClass<TCHAR>());
    }
    NFA(NFA<TCHAR>&& old)
        : NFABase<NFAState<TCHAR> >(old)
    {
    }
    NFA(const NFA<TCHAR>& old)
        : NFABase<NFAState<TCHAR> >(old)
    {
    }
    virtual ~NFA()
    {
    }
    bool epsilon_closure_sub(std::set<int>& closure, int index) const
    {
        //Always include myself
        closure.insert(index);

        bool long_flag = false;
        IndexVector et = m_states[index].epsilon_transitions(long_flag);

        for (int dest : et)
        {
            epsilon_closure_sub(closure, dest);
        }
        return long_flag;
    }
    /*!
     * @brief Collect the epsilon closure around the state
     */
    std::set<int> epsilon_closure(int index, bool& long_flag) const
    {
        std::set<int> ret;

        long_flag = epsilon_closure_sub(ret, index);

        return ret;
    }
    std::set<int> epsilon_closure(const std::set<int>& indices, bool& long_flag) const
    {
        std::set<int> ret;

        for (int index : indices)
        {
            long_flag |= epsilon_closure_sub(ret, index);
        }

        return ret;
    }
    /*!
     * @brief Tests if the NFA accepts the given string
     */
    bool run(const std::basic_string<TCHAR>& seq, int index = 0, int input_pos = 0) const
    {
        if (index == m_states.size() - 1 && seq.size() == input_pos)
        {
            return true;
        }
        else
        {
            for (const auto& tr : get_transitions(index))
            {
                if (tr.is_epsilon())
                {
                    if (run(seq, tr.dest(), input_pos))
                        return true;
                }
                else if (tr.label().includes(seq[input_pos]))
                {
                    if (run(seq, tr.dest(), input_pos + 1))
                        return true;
                }
            }
            return false;
        }
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
    void add_transition_to(const CharClass<TCHAR>& cc, int dest, bool long_flag = false)
    {
        m_states.back().add_transition(cc, dest, long_flag);
    }
    /*!
    * @brief Add a transition to the final state from another state.
    */
    void add_transition_from(const CharClass<TCHAR>& cc, int src)
    {
        m_states[src].add_transition(cc, m_states.size() - 1);
    }
    const std::vector<NFATransition<TCHAR> >& get_transitions(int index) const
    {
        return m_states[index].get_transitions();
    }
};
}
