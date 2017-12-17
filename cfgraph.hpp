#pragma once

#include <vector>
#include <list>

#include "charclass.hpp"

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
	RDFATransition()
	{

	}
	virtual ~RDFATransition()
	{
	}
};
template<typename TCHAR>
class RDFAState
{
	std::vector<RDFATransition<TCHAR> > m_transitions;
public:
	RDFAState()
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
	std::list<RDFABasicBlock<TCHAR> > m_children;
	std::vector<CharClass<TCHAR> > m_forward;
	CharClass<TCHAR> m_loop;
public:
    RDFABasicBlock(RDFABBType type)
		: m_type(type)
    {
    }
    virtual ~RDFABasicBlock()
    {
    }
};
template<typename TCHAR>
class RestrictedDFA
{
	RDFABasicBlock m_root;
public:
    RestrictedDFA(const NFA<TCHAR>& nfa)
		: m_root(RDFABBType::Conditional)
    {
    }
    virtual ~RestrictedDFA()
    {
    }
};
}
