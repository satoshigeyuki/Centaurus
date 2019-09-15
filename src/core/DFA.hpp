#pragma once

#include <utility>
#include <set>
#include <algorithm>
#include <vector>
#include <unordered_map>

#include "NFA.hpp"

namespace Centaurus
{
template<typename TCHAR> using DFATransition = NFATransition<TCHAR>;
template<typename TCHAR> class DFAState : public NFABaseState<TCHAR, IndexVector>
{
    bool m_long, m_accept;
public:
	using NFABaseState<TCHAR, IndexVector>::get_transitions;
	using NFABaseState<TCHAR, IndexVector>::m_transitions;
    using NFABaseState<TCHAR, IndexVector>::hash;
    using NFABaseState<TCHAR, IndexVector>::operator==;
    using NFABaseState<TCHAR, IndexVector>::sort;
	DFAState(const IndexVector& label, bool long_flag = false)
		: NFABaseState<TCHAR, IndexVector>(label), m_long(long_flag), m_accept(false) {}
    DFAState(const DFAState<TCHAR>& state, std::vector<DFATransition<TCHAR> >&& transitions)
        : NFABaseState<TCHAR, IndexVector>(state, std::move(transitions)), m_long(state.m_long), m_accept(state.m_accept) {}
	bool is_accept_state() const
	{
		return m_accept;
	}
	virtual void print(std::wostream& os, int from) const override
	{
		for (const auto& t : m_transitions)
		{
			os << L"S" << from << L" -> " << L"S" << t.dest() << L" [ label=\"";
			os << t.label();
			os << L"\" ];" << std::endl;
		}
	}
    void set_long(bool long_flag = true)
    {
        m_long |= long_flag;
    }
    bool is_long() const
    {
        return m_long;
    }
	void set_accept(bool accept_flag = true)
	{
		m_accept |= accept_flag;
	}
	bool is_accept() const
	{
		return m_accept;
	}
};

template<typename TCHAR> class DFA : public NFABase<DFAState<TCHAR> >
{
	using NFABase<DFAState<TCHAR> >::m_states;
	/*!
	 * @brief Create a state with a label if it does not exist
	 */
	int add_state(const std::set<int>& label, bool long_flag = false)
	{
		for (unsigned int i = 0; i < m_states.size(); i++)
		{
			if (label.size() == m_states[i].label().size() && std::equal(label.cbegin(), label.cend(), m_states[i].label().cbegin()))
			{
                m_states[i].set_long(long_flag);
				return i;
			}
		}
		m_states.emplace_back(IndexVector(label.cbegin(), label.cend()), long_flag);
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
            bool long_flag = false;
			std::set<int> ec = nfa.epsilon_closure(item.second, long_flag);

			int new_index = add_state(ec, long_flag);

			//std::cout << "S" << index << "->" << new_index << std::endl;

			m_states[index].add_transition(item.first, new_index);
		}

		for (int i = initial_index; i < m_states.size(); i++)
		{
			fork_states(i, nfa);
		}
	}
public:
	DFA(const NFA<TCHAR>& nfa, bool optimize_flag = true)
	{
		//Start by collecting the epsilon closure of start state
        bool long_flag = false;
		std::set<int> ec0 = nfa.epsilon_closure(0, long_flag);

		//Create the start state
		m_states.emplace_back(IndexVector(ec0.cbegin(), ec0.cend()), long_flag);

		//Recursively construct the DFA
		fork_states(0, nfa);

		int accept_state_index = nfa.get_state_num() - 1;

		for (auto& state : m_states)
		{
			if (state.label().includes(accept_state_index))
			{
				//state.add_transition(CharClass<TCHAR>(), -1);
				state.set_accept();
			}

            state.sort();
		}

        if (optimize_flag)
            minimize();
	}
	DFA()
	{
	}
    DFA(DFA<TCHAR>&& dfa)
        : NFABase<DFAState<TCHAR> >(dfa)
    {
    }
    DFA(const DFA<TCHAR>& dfa)
        : NFABase<DFAState<TCHAR> >(dfa)
    {
    }
	virtual ~DFA()
	{
	}
	virtual void print(std::wostream& os, const std::wstring& graph_name) const override
	{
		os << L"digraph " << graph_name << L" {" << std::endl;
		os << L"rankdir=\"LR\";" << std::endl;
		os << L"graph [ charset=\"UTF-8\", style=\"filled\" ];" << std::endl;
		os << L"node [ style=\"solid,filled\" ];" << std::endl;
		os << L"edge [ style=\"solid\" ];" << std::endl;

		os << L"SS [ style=\"invisible\" ];" << std::endl;

		for (unsigned int i = 0; i < m_states.size(); i++)
		{
			os << L"S" << i << L" [ label=\"";
			print_state(os, i);
			if (m_states[i].is_accept_state())
				os << L"\", shape=doublecircle";
			else
				os << L"\", shape=circle";
            if (m_states[i].is_long())
                os << L", penwidth=2 ];" << std::endl;
            else
                os << L" ];" << std::endl;
		}

		os << L"SS -> S0;" << std::endl;

		for (unsigned int i = 0; i < m_states.size(); i++)
		{
			m_states[i].print(os, i);
		}

		os << L"}" << std::endl;
	}
	virtual void print_state(std::wostream& os, int index) const override
	{
		os << m_states[index].label();
	}
	bool run(const std::basic_string<TCHAR>& seq, int index = 0, int input_pos = 0) const
	{
		const DFAState<TCHAR>& state = m_states[index];
		if (input_pos == seq.size())
		{
			return state.is_accept();
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
		return m_states[index].is_accept();
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
    void filter_nodes(const std::vector<bool>& mask)
    {
        std::vector<DFAState<TCHAR> > nodes;
        std::vector<int> index_map(m_states.size());

        assert(m_states.size() == mask.size());

        for (int src_index = 0, dest_index = 0; src_index < m_states.size(); src_index++)
        {
            if (mask[src_index])
            {
                std::vector<DFATransition<TCHAR> > filtered;
                for (const auto& t : m_states.at(src_index).get_transitions())
                {
                    if (mask[t.dest()])
                    {
                        filtered.push_back(t);
                    }
                }
                nodes.emplace_back(m_states.at(src_index), std::move(filtered));
                index_map[src_index] = dest_index;
                dest_index++;
            }
            else
            {
                index_map[src_index] = -1;
            }
        }

        for (auto& n : nodes)
        {
            for (auto& t : n.get_transitions())
            {
                t.dest(index_map[t.dest()]);
            }
        }
        m_states = std::move(nodes);
    }
    void redirect_transitions(int from, int to)
    {
        for (auto& s : m_states)
        {
            for (auto& t : s.get_transitions())
            {
                if (t.dest() == from)
                {
                    t.dest(to);
                }
            }
        }
    }
    void minimize()
    {
        std::vector<bool> mask(m_states.size(), true);
        std::vector<int> remap(m_states.size(), -1);

        std::unordered_map<DFAState<TCHAR>, int> equiv_map;

        for (int index = 0; index < m_states.size(); index++)
        {
            auto p = equiv_map.find(m_states[index]);
            if (p != equiv_map.end())
            {
                m_states[p->second].set_long(m_states[index].is_long());
                remap[index] = p->second;
                mask[index] = false;
            }
            else
            {
                equiv_map.emplace(m_states[index], index);
            }
        }

        for (int index = 0; index < m_states.size(); index++)
        {
            if (remap[index] >= 0)
            {
                redirect_transitions(index, remap[index]);
            }
        }
        filter_nodes(mask);
    }
};
}

namespace std
{
template<typename TCHAR>
struct hash<Centaurus::DFAState<TCHAR> >
{
    size_t operator()(const Centaurus::DFAState<TCHAR>& s) const
    {
        return s.hash();
    }
};
template<typename TCHAR>
struct equal_to<Centaurus::DFAState<TCHAR> >
{
    bool operator()(const Centaurus::DFAState<TCHAR>& x, const Centaurus::DFAState<TCHAR>& y) const
    {
        return x == y;
    }
};
}
