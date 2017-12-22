#pragma once

#include <vector>
#include <list>
#include <set>

#include "charclass.hpp"
#include "nfa.hpp"

namespace Centaurus
{
enum class RDFABBType
{
	Sequence,
	Conditional
};
template<typename TCHAR>
class RDFATransition
{
	CharClass<TCHAR> m_label;
public:
	RDFATransition(const CharClass<TCHAR>& label)
		: m_label(label)
	{
	}
	virtual ~RDFATransition()
	{
	}
	const CharClass<TCHAR>& label() const
	{
		return m_label;
	}
};
template<typename TCHAR>
class RDFAState
{
	std::set<int> m_label;
public:
	RDFAState(const std::set<int>& label)
		: m_label(label)
	{

	}
	virtual ~RDFAState()
	{

	}
};
template<typename TCHAR>
class RDFABasicBlock
{
	RDFABBType m_type;
	//! Children may form either a selection or a sequence according to the type.
	std::list<RDFABasicBlock<TCHAR> > m_children;
	//! Transition INTO this block, not OUT OF.
	RDFATransition<TCHAR> m_entry;
	//! Transition pointing back to the beginning, if any.
	RDFATransition<TCHAR> m_loop;
	//! Each BB contains only one state.
	RDFAState<TCHAR> m_state;
public:
    RDFABasicBlock(RDFABBType type)
		: m_type(type)
    {
    }
	RDFABasicBlock(RDFABBType type, const CharClass<TCHAR>& entry, const RDFAState<TCHAR>& state)
		: m_type(type), m_entry(entry), m_state(state)
	{
	}
	void add_alternative(const RDFABasicBlock<TCHAR>& bb)
	{
		m_children.push_back(bb);
	}
    virtual ~RDFABasicBlock()
    {
    }
};
template<typename TCHAR>
class RestrictedDFA
{
	//! The root BB whose type is always Conditional.
	RDFABasicBlock<TCHAR> m_root;
	void construct_r(RDFABasicBlock<TCHAR>& bb, const NFA<TCHAR>& nfa)
	{
		
	}
public:
    RestrictedDFA(const NFA<TCHAR>& nfa)
		: m_root(RDFABBType::Conditional)
    {
		//Start by collecting the epsilon closure of start state
		std::set<int> ec0 = nfa.epsilon_closure(0);

		m_root.add_alternative(RDFABasicBlock<TCHAR>(RDFABBType::Sequence, CharClass<TCHAR>(), RDFAState<TCHAR>(ec0)));
    
		construct_r(m_root, nfa);
	}
    virtual ~RestrictedDFA()
    {
    }
};
}
