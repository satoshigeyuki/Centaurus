#pragma once

#include <vector>

namespace Centaur
{
template<typename TCHAR> class Range
{
    TCHAR m_start, m_end;
public:
    Range(TCHAR start, TCHAR end)
        : m_start(start), m_end(end)
    {
        assert(start <= end);
    }
    virtual ~Range()
    {
    }
    bool operator<(const Range<TCHAR>& r) const
    {
        return m_end <= r.start;
    }
    bool operator>(const Range<TCHAR>& r) const
    {
        return r.end <= m_start;
    }
    bool overlaps(const Range<TCHAR>& r) const
    {
        return m_start < r.end && r.start < m_end;
    }
};

template<typename TCHAR> class NFACharClass
{
    std::vector<Range<TCHAR> > m_ranges;
public:
    NFACharClass()
    {
    }
    virtual ~NFACharClass()
    {
    }
    bool is_epsilon() const
    {
        return m_ranges.empty();
    }
    NFACharClass<TCHAR>& operator|=(const Range<TCHAR>& r)
    {
        std::vector<Range<TCHAR> > new_ranges;

        auto i = m_ranges.cbegin();

        for (; i != m_ranges.cend(); i++)
        {
            if (i->overlaps(r))
            {
                new_ranges.push_back(i->merge(r));
                i++;
                break;
            }
            else
            {
                new_ranges.push_back(*i);
            }
        }
        new_ranges.insert(new_ranges.end(), i, m_ranges.cend());

        m_ranges = std::move(new_ranges);
        return *this;
    }
};

template<typename TCHAR> class NFATransition
{
    NFACharClass<TCHAR> m_label;
    const NFAState<TCHAR>& m_dest;
public:
    NFATransition(const NFAState<TCHAR>& dest)
        : m_dest(dest)
    {
    }
    virtual ~NFATransition()
    {
    }
};

template<typename TCHAR> class NFAState
{
    std::vector<NFATransition<TCHAR> > m_transitions;
public:
    NFAState()
    {
    }
    virtual ~NFAState()
    {
    }
};

template<typename TCHAR> class NFA
{
public:
    NFA()
    {
    }
    virtual ~NFA()
    {
    }
    NFA operator+(const NFA& nfa)
    {
    }
};
}
