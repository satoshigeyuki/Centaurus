#pragma once

#include <iostream>
#include <vector>
#include <limits>
#include <string>
#include <locale>
#include <algorithm>

#include "stream.hpp"
#include "util.hpp"

namespace Centaurus
{
/*!
 * @brief Represents an half-open range of type TCHAR.
 *        This class is unable to handle the maximum character of type TCHAR, because of the half-open representation.
 *        This limitation should not be problematic because most character sets reserve that code point.
 */
template<typename TCHAR> class Range
{
    TCHAR m_start, m_end;

    template<typename T> friend std::ostream& operator<<(std::ostream& os, const Range<T>& r);
public:
    Range(TCHAR start, TCHAR end)
        : m_start(start), m_end(end)
    {
    }
    Range()
        : m_start(0), m_end(0)
    {
    }
    static Range<TCHAR> make_from_wide(char16_t start, char16_t end)
    {
        return Range<TCHAR>(wide_to_target<TCHAR>(start), wide_to_target<TCHAR>(end));
    }
    Range(const Range<TCHAR>& r)
        : m_start(r.m_start), m_end(r.m_end)
    {
    }
    virtual ~Range()
    {
    }
    bool operator<(const Range<TCHAR>& r) const
    {
        return m_end < r.m_start;
    }
    bool operator>(const Range<TCHAR>& r) const
    {
        return r.m_end < m_start;
    }
    bool operator==(const Range<TCHAR>& r) const
    {
        return m_start == r.m_start && m_end == r.m_end;
    }
    bool overlaps(const Range<TCHAR>& r) const
    {
        return m_start <= r.m_end && r.m_start <= m_end;
    }
    bool includes(const Range<TCHAR>& r) const
    {
        return m_start <= r.m_start && r.m_end <= m_end;
    }
    Range<TCHAR> merge(const Range<TCHAR>& r) const
    {
        return Range<TCHAR>(std::min(m_start, r.m_start), std::max(m_end, r.m_end));
    }
    Range<TCHAR> intersect(const Range<TCHAR>& r) const
    {
        return Range<TCHAR>(std::max(m_start, r.m_start), std::min(m_emd, r.m_end));
    }
    TCHAR start() const
    {
        return m_start;
    }
    TCHAR end() const
    {
        return m_end;
    }
};

template<typename TCHAR>
std::ostream& operator<<(std::ostream& os, const Range<TCHAR>& r)
{
    os << '[' << r.start() << ", " << r.end() - 1 << ')';
    return os;
}

template<typename TCHAR> class CharClass
{
    std::vector<Range<TCHAR> > m_ranges;

    template<typename T> friend std::ostream& operator<<(std::ostream& os, const CharClass<T>& cc);

    void parse(Stream& stream);
public:
    CharClass()
    {
    }
    CharClass(Stream& stream)
    {
        parse(stream);
    }
    CharClass(char16_t ch)
    {
        m_ranges.push_back(Range<TCHAR>::make_from_wide(ch, ch + 1));
    }
    CharClass(char16_t start, char16_t end)
    {
        m_ranges.push_back(Range<TCHAR>::make_from_wide(start, end));
    }
    CharClass(const CharClass<TCHAR>& cc)
        : m_ranges(cc.m_ranges)
    {
    }
    CharClass(const Range<TCHAR>& r)
    {
        m_ranges.push_back(r);
    }
    virtual ~CharClass()
    {
    }
    static CharClass<TCHAR> make_star()
    {
        return CharClass<TCHAR>(0, std::numeric_limits<TCHAR>::max());
    }
    bool is_epsilon() const
    {
        return m_ranges.empty();
    }
    void invert()
    {
        TCHAR last_end = 0;

        for (auto& i : m_ranges)
        {
            TCHAR new_end = i.end();
            i = Range<TCHAR>(last_end, i.start());
            last_end = new_end;
        }

        if (last_end < std::numeric_limits<TCHAR>::max())
            m_ranges.emplace_back(last_end, std::numeric_limits<TCHAR>::max());
    }
    CharClass<TCHAR> operator~() const
    {
        CharClass<TCHAR> cc(*this);

        cc.invert();

        return cc;
    }
    CharClass<TCHAR> operator|(Range<TCHAR> r) const
    {
        CharClass<TCHAR> cc;

        auto i = m_ranges.cbegin();

        for (; i != m_ranges.cend() && !i->overlaps(r); i++)
            ;
        
        cc.m_ranges.insert(cc.m_ranges.end(), m_ranges.cbegin(), i);

        for (; i != m_ranges.cend() && i->overlaps(r); i++)
        {
            r = r.merge(*i);
        }

        cc.m_ranges.push_back(r);

        cc.m_ranges.insert(cc.m_ranges.end(), i, m_ranges.cend());

        return cc;
    }
    CharClass<TCHAR>& operator|=(const Range<TCHAR>& r)
    {
        return *this = *this | r;
    }
    CharClass<TCHAR> operator|(const CharClass<TCHAR>& cc) const;
    CharClass<TCHAR>& operator|=(const CharClass<TCHAR>& cc)
    {
        return *this = *this | cc;
    }
    bool operator==(const CharClass<TCHAR>& cc) const
    {
        return std::equal(m_ranges.cbegin(), m_ranges.cend(), cc.m_ranges.cbegin());
    }
    CharClass<TCHAR> operator&(const CharClass<TCHAR>& cc) const;
    /*!
     * @brief Return a soup of all the boundaries in the class.
     */
    IndexVector collect_borders() const
    {
        IndexVector ret(2 * m_ranges.size(), true);

        for (const auto& r : m_ranges)
        {
            ret.push_back(r.start());
            ret.push_back(r.end());
        }

        return ret;
    }
    void collect_borders(std::set<TCHAR>& dest) const
    {
        for (const auto& r : m_ranges)
        {
            dest.insert(r.start());
            dest.insert(r.end());
        }
    }
    bool includes(const Range<TCHAR>& r) const
    {
        for (const auto& mr : m_ranges)
        {
            if (mr.includes(r)) return true;
        }
        return false;
    }
    bool overlaps(const CharClass<TCHAR>& cc) const
    {
        auto i = m_ranges.cbegin();
        auto j = cc.m_ranges.cbegin();

        while (i != m_ranges.cend() && j != cc.m_ranges.cend())
        {
            const Range<TCHAR>& ri = *i;
            const Range<TCHAR>& rj = *j;

            if (ri < rj)
            {
                i++;
            }
            else if (rj < ri)
            {
                j++;
            }
            else
            {
                return true;
            }
        }
        return false;
    }
};
}
