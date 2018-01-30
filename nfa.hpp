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

namespace Centaurus
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
    void print_subgraph(std::ostream& os, int from, const std::string& prefix) const
    {
        for (const auto& t : m_transitions)
        {
            os << prefix << "_S" << from << " -> " << prefix << "_S" << t.dest() << " [ label=\"";
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
    virtual void print_state(std::ostream& os, int index) const
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
    void print_subgraph(std::ostream& os, const std::string& prefix) const
    {
        os << "subgraph cluster_" << prefix << " {" << std::endl;
        for (unsigned int i = 0; i < m_states.size(); i++)
        {
            os << prefix << "_S" << i << " [ label=\"";
            print_state(os, i);
            os << "\", shape=circle ];" << std::endl;
        }
        for (unsigned int i = 0; i < m_states.size(); i++)
        {
            m_states[i].print_subgraph(os, i, prefix);
        }
        os << "}" << std::endl;
    }
    std::string get_entry(const std::string& prefix) const
    {
        return prefix + "_S0";
    }
    std::string get_exit(const std::string& prefix) const
    {
        return prefix + "_S" + std::to_string(m_states.size() - 1);
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
		emplace_back(CharClass<TCHAR>(r), closure);
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
		if (tr.is_epsilon()) return;

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
        for (char16_t ch = stream.get(); ch != u')' && ch != u'/' && ch != u'\0'; ch = stream.get())
        {
            NFA<TCHAR> nfa;
            nfa.parse_selection(stream, ch);
            concat(nfa);
        }
    }
    void parse_unit(Stream& stream, char16_t ch)
    {
        switch (ch)
        {
        case u'\\':
            ch = stream.get();
            switch (ch)
            {
            case u'[':
            case u']':
            case u'+':
            case u'*':
            case u'{':
            case u'}':
            case u'?':
            case u'\\':
            case u'|':
            case u'(':
            case u')':
            case u'.':
                add_state(CharClass<TCHAR>(ch));
                break;
            default:
                throw stream.unexpected(ch);
            }
            break;
        case u'[':
            add_state(CharClass<TCHAR>(stream));
            break;
        case u'.':
            add_state(CharClass<TCHAR>::make_star());
            break;
        case u'(':
            concat(NFA<TCHAR>(stream));
            break;
        default:
            add_state(CharClass<TCHAR>(ch));
            break;
        }
        ch = stream.peek();
        switch (ch)
        {
        case u'+':
            stream.discard();
            //Add an epsilon transition to the last state
            add_transition_to(CharClass<TCHAR>(), 1);
            break;
        case u'*':
            stream.discard();
            //Add an epsilon transition to the last state
            add_transition_to(CharClass<TCHAR>(), 1);
            //Add a confluence
            add_state(CharClass<TCHAR>());
            //Add a skipping transition
            add_transition_from(CharClass<TCHAR>(), 1);
            break;
        case u'?':
            stream.discard();
            //Add a skipping transition
            add_transition_from(CharClass<TCHAR>(), 1);
            break;
        }
    }
    void parse_selection(Stream& stream, char16_t ch)
    {
        parse_unit(stream, ch);

        while (true)
        {
            ch = stream.peek();

            if (ch != u'|')
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
    virtual ~NFA()
    {
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
	const std::vector<NFATransition<TCHAR> >& get_transitions(int index) const
	{
		return m_states[index].get_transitions();
	}
};
}
