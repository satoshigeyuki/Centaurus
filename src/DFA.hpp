#pragma once

#include <utility>
#include <set>
#include <algorithm>
#include <vector>

#include "NFA.hpp"

namespace Centaurus
{
template<typename TCHAR> using DFATransition = NFATransition<TCHAR>;
template<typename TCHAR> class DFAState : public NFABaseState<TCHAR, IndexVector>
{
public:
	using NFABaseState<TCHAR, IndexVector>::get_transitions;
	DFAState(const IndexVector& label)
		: NFABaseState<TCHAR, IndexVector>(label) {}
	bool is_accept_state() const
	{
		for (const auto& tr : get_transitions())
		{
			if (tr.dest() == -1) return true;
		}
		return false;
	}
};

template<typename TCHAR> class DFA : public NFABase<DFAState<TCHAR> >
{
	using NFABase<DFAState<TCHAR> >::m_states;
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
		NFADepartureSetFactory<TCHAR> ds_factory;

		for (int i : m_states[index].label())
		{
			for (const auto& tr : nfa.get_transitions(i))
			{
				ds_factory.add(tr);
			}
		}

		NFADepartureSet<TCHAR> deptset = ds_factory.build_departure_set();

		int initial_index = m_states.size();
		for (const auto& item : deptset)
		{
			std::set<int> ec = nfa.epsilon_closure(item.second);

			int new_index = add_state(ec);

			//std::cout << "S" << index << "->" << new_index << std::endl;

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

		int accept_state_index = nfa.get_state_num() - 1;

		for (auto& state : m_states)
		{
			if (state.label().includes(accept_state_index))
			{
				state.add_transition(CharClass<TCHAR>(), -1);
			}
		}
	}
	DFA()
	{
	}
	virtual ~DFA()
	{
	}
	virtual void print_state(std::ostream& os, int index)
	{
		os << m_states[index].label();
	}
	bool run(const std::basic_string<TCHAR>& seq, int index = 0, int input_pos = 0) const
	{
		const DFAState<TCHAR>& state = m_states[index];
		if (input_pos == seq.size())
		{
			for (const auto& tr : state.get_transitions())
			{
				if (tr.dest() == -1) return true;
			}
			return false;
		}
		else
		{
			for (const auto& tr : state.get_transitions())
			{
				if (tr.label().includes(seq[input_pos]))
				{
					return run(seq, tr.dest(), input_pos + 1);
				}
			}
			return false;
		}
	}
	const std::vector<DFATransition<TCHAR> >& get_transitions(int index) const
	{
		return m_states[index].get_transitions();
	}
	bool is_accept_state(int index) const
	{
		for (const auto& tr : get_transitions(index))
		{
			if (tr.dest() == -1) return true;
		}
		return false;
	}
	typename std::vector<DFAState<TCHAR> >::const_iterator begin() const
	{
		return m_states.cbegin();
	}
	typename std::vector<DFAState<TCHAR> >::const_iterator end() const
	{
		return m_states.cend();
	}
	const DFAState<TCHAR>& operator[](int index) const
	{
		return m_states[index];
	}
};
}
