#pragma once

#include <iostream>
#include <vector>
#include <set>
#include <utility>

#include "CompositeATN.hpp"
#include "NFA.hpp"
#include "Util.hpp"
#include "Exception.hpp"

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
public:
	using NFABaseState<TCHAR, CATNClosure>::m_label;
	using NFABaseState<TCHAR, CATNClosure>::m_transitions;
    using NFABaseState<TCHAR, CATNClosure>::hash;
    using NFABaseState<TCHAR, CATNClosure>::operator==;
    using NFABaseState<TCHAR, CATNClosure>::sort;
	virtual void print(std::wostream& os, int from) const override
	{
		for (const auto& t : m_transitions)
		{
			os << L"S" << from << L" -> ";
            if (t.dest() < 0)
            {
                os << L"A" << -t.dest();
            }
            else
            {
                os << L"S" << t.dest();
            }
            os << L" [ label=\"";
			os << t.label();
			os << L"\" ];" << std::endl;
		}
	}
	int get_color() const
	{
		int color = 0;
		for (const auto& p : m_label)
		{
			if (color == 0)
			{
				color = p.color();
			}
			else
			{
				if (color != p.color() && p.color() != 0)
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
    LDFAState(const LDFAState<TCHAR>& state, std::vector<LDFATransition<TCHAR> >&& transitions)
        : NFABaseState<TCHAR, CATNClosure>(state, std::move(transitions))
    {
    }
	virtual ~LDFAState()
	{
	}
	const std::vector<LDFATransition<TCHAR> >& get_transitions() const
	{
		return m_transitions;
	}
	std::vector<LDFATransition<TCHAR> >& get_transitions()
	{
		return m_transitions;
	}
};

template<typename TCHAR>
class LookaheadDFA : public NFABase<LDFAState<TCHAR> >
{
	using NFABase<LDFAState<TCHAR> >::m_states;
    using NFABase<LDFAState<TCHAR> >::print_state;
private:
	static int get_closure_color(const CATNClosure& closure)
	{
		int color = 0;
		for (const auto& p : closure)
		{
			if (color == 0)
			{
				color = p.color();
			}
			else
			{
				if (color != p.color() && p.color() != 0)
					return -1;
			}
		}
		return color;
	}
	int add_state(const CATNClosure& label)
	{
		for (unsigned int i = 0; i < m_states.size(); i++)
		{
			if (label == m_states[i].label())
			{
				return i;
			}
		}
		m_states.emplace_back(label);
		return m_states.size() - 1;
	}
	/*!
	 * @brief Construct LDFA states around the current state
	 */
	void fork_closures(const CompositeATN<TCHAR>& catn, int index)
	{
		const LDFAState<TCHAR>& state = m_states[index];

		//First, check if the state in question is already single-colored
		/*if (state.get_color() > 0)
		{
			std::cout << "Monochromatic " << index << std::endl;
			return;
		}*/

		CATNDepartureSet<TCHAR> deptset = catn.build_departure_set(state.label());

		/*std::wofstream deptsetLog("deptset.log", std::ios::app);

		deptsetLog << deptset << std::endl;

		deptsetLog.close();*/

		//std::cout << deptset << std::endl;

		//Add new states to the LDFA
		int initial_index = m_states.size();

		for (const auto& item : deptset)
		{
			int color = get_closure_color(item.second);

			if (color > 0)
			{
				m_states[index].add_transition(item.first, -color);
			}
			else
			{
				CATNClosure ec = catn.build_closure(item.second);

				int new_index = add_state(ec);

				m_states[index].add_transition(item.first, new_index);
			}
		}

		//Recursively construct the closures for all the newly added DFA states
		for (unsigned int i = initial_index; i < m_states.size(); i++)
		{
			fork_closures(catn, i);
		}
	}
public:
	LookaheadDFA(const CompositeATN<TCHAR>& catn, const ATNPath& origin, bool optimize_flag = true)
	{
		m_states.emplace_back(catn.build_root_closure(origin));

		fork_closures(catn, 0);

        for (auto& state : m_states)
            state.sort();

        if (optimize_flag)
            minimize();
	}
    LookaheadDFA(LookaheadDFA<TCHAR>&& old)
        : NFABase<LDFAState<TCHAR> >(old)
    {
    }
    LookaheadDFA(const LookaheadDFA<TCHAR>& old)
        : NFABase<LDFAState<TCHAR> >(old)
    {
    }
	virtual ~LookaheadDFA()
	{
	}
	const std::vector<LDFATransition<TCHAR> >& get_transitions(int index) const
	{
		return m_states[index].get_transitions();
	}
	int get_state_num() const
	{
		return m_states.size();
	}
	int get_color_num() const
	{
		int max_color_index = 0;

		for (const auto& state : m_states)
		{
			for (const auto& tr : state.get_transitions())
			{
				if (tr.dest() < 0)
				{
					max_color_index = std::max(max_color_index, -tr.dest());
				}
			}
		}
		return max_color_index;
	}
	int run(const std::basic_string<TCHAR>& seq, int index = 0, int input_pos = 0) const
	{
		if (input_pos == seq.size())
		{
			return -1;
		}
		else
		{
			const LDFAState<TCHAR>& state = m_states[index];

			for (const auto& tr : state.get_transitions())
			{
				if (tr.label().includes(seq[input_pos]))
				{
					if (tr.dest() < 0)
					{
						return -tr.dest();
					}
					return run(seq, tr.dest(), input_pos + 1);
				}
			}
			return -1;
		}
	}
	const LDFAState<TCHAR>& operator[](int index) const
	{
		return m_states[index];
	}
	virtual void print(std::wostream& os, const std::wstring& graph_name) const override
	{
		os << L"digraph " << graph_name << L" {" << std::endl;
		os << L"rankdir=\"LR\";" << std::endl;
		os << L"graph [ charset=\"UTF-8\", style=\"filled\" ];" << std::endl;
		os << L"node [ style=\"solid,filled\" ];" << std::endl;
		os << L"edge [ style=\"solid\" ];" << std::endl;

        int num_colors = get_color_num();

        for (int i = 1; i <= num_colors; i++)
        {
            os << L"A" << i << L" [ label=\"";
            os << i;
            os << L"\", shape=doublecircle ];" << std::endl;
        }

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
    void filter_nodes(const std::vector<bool>& mask)
    {
        std::vector<LDFAState<TCHAR> > nodes;
        std::vector<int> index_map(m_states.size());

        assert(m_states.size() == mask.size());

        for (int src_index = 0, dest_index = 0; src_index < m_states.size(); src_index++)
        {
            if (mask[src_index])
            {
                std::vector<LDFATransition<TCHAR> > filtered;
                for (const auto& t : m_states.at(src_index).get_transitions())
                {
                    if (t.dest() < 0)
                    {
                        filtered.push_back(t);
                    }
                    else if (mask[t.dest()])
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
                if (t.dest() >= 0)
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

        std::unordered_map<LDFAState<TCHAR>, int> equiv_map;

        for (int index = 0; index < m_states.size(); index++)
        {
            auto p = equiv_map.find(m_states[index]);
            if (p != equiv_map.end())
            {
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
struct hash<Centaurus::LDFAState<TCHAR> >
{
    size_t operator()(const Centaurus::LDFAState<TCHAR>& s) const
    {
        return s.hash();
    }
};
template<typename TCHAR>
struct equal_to<Centaurus::LDFAState<TCHAR> >
{
    bool operator()(const Centaurus::LDFAState<TCHAR>& x, const Centaurus::LDFAState<TCHAR>& y) const
    {
        return x == y;
    }
};
}
