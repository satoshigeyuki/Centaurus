#pragma once

#include <iostream>
#include <vector>
#include <limits>
#include <string>
#include <locale>
#include <algorithm>
#include <tuple>
#include <set>

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
        return Range<TCHAR>(std::max(m_start, r.m_start), std::min(m_end, r.m_end));
    }
    TCHAR start() const
    {
        return m_start;
    }
    TCHAR end() const
    {
        return m_end;
    }
    void start(TCHAR ch)
    {
        m_start = ch;
    }
    void end(TCHAR ch)
    {
        m_end = ch;
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
            if (*i < *j)
            {
                i++;
            }
            else if (*j < *i)
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
    void append(const Range<TCHAR>& r)
    {
        if (m_ranges.empty())
        {
            m_ranges.push_back(r);
        }
        else
        {
            if (m_ranges.back().end() == r.start())
            {
                m_ranges.back().end(r.end());
            }
            else
            {
                m_ranges.push_back(r);
            }
        }
    }
    /*!
     * @brief Take the differences and the intersection between the character classes in one pass
     */
    std::array<CharClass<TCHAR>, 3> diff_and_int(const CharClass<TCHAR>& cc) const
    {
        //Returns 3 character classes
        //ret[0] => *this & ~cc
        //ret[1] => ~(*this) & cc
        //ret[2] => *this & cc
        std::array<CharClass<TCHAR>, 3> ret;

        auto i = m_ranges.cbegin();
        auto j = cc.m_ranges.cbegin();
        
        Range<TCHAR> ri, rj;

        if (i != m_ranges.cend())
            ri = *i;
        if (j != cc.m_ranges.cend())
            rj = *j;

        while (i != m_ranges.cend() && j != cc.m_ranges.cend())
        {
            if (ri < rj)
            {
                ret[0].append(ri);
                if (++i != m_ranges.cend())
                    ri = *i;
            }
            else if (ri > rj)
            {
                ret[1].append(rj);
                if (++j != cc.m_ranges.cend())
                    rj = *j;
            }
            else
            {
                if (ri.start() < rj.start())
                {
                    //When ri begins earlier than rj
                    ret[0].append(Range<TCHAR>(ri.start(), rj.start()));
                    ri.start(rj.start());
                }
                else if (rj.start() < ri.start())
                {
                    //When rj begins earlier than ri
                    ret[1].append(Range<TCHAR>(rj.start(), ri.start()));
                    rj.start(ri.start());
                }
                else
                {
                    //ri.start() == rj.start()
                    if (ri.end() > rj.end())
                    {
                        //When ri ends later than rj
                        ret[2].append(Range<TCHAR>(rj.start(), rj.end()));
                        ri.start(rj.end());
                        if (++j != cc.m_ranges.cend())
                            rj = *j;
                    }
                    else if (ri.end() < rj.end())
                    {
                        //When rj ends later than ri
                        ret[2].append(Range<TCHAR>(ri.start(), ri.end()));
                        rj.start(ri.end());
                        if (++i != m_ranges.cend())
                            ri = *i;
                    }
                    else
                    {
                        //When ri equals to rj
                        ret[2].append(Range<TCHAR>(ri.start(), rj.end()));
                        if (++i != m_ranges.cend())
                            ri = *i;
                        if (++j != cc.m_ranges.cend())
                            rj = *j;
                    }
                }
            }
        }

        while (i != m_ranges.cend())
        {
            ret[0].append(ri);
            if (++i != m_ranges.cend())
                ri = *i;
        }
        while (j != cc.m_ranges.cend())
        {
            ret[1].append(rj);
            if (++j != cc.m_ranges.cend())
                rj = *j;
        }

        std::cout << ret[0] << std::endl;
        std::cout << ret[1] << std::endl;
        std::cout << ret[2] << std::endl;

        return ret;
    }
    /*CharClass<TCHAR> exclude(const CharClass<TCHAR>& cc) const
    {
        CharClass<TCHAR> ret;

        auto i = m_ranges.cbegin();
        auto j = cc.m_ranges.cbegin();

        while (i != m_ranges.cend() && j != cc.m_ranges.cend())
        {
            if (*i < *j)
            {
                // *i is not included in any of cc.m_ranges
                ret.m_ranges.push_back(*i);
                i++;
            }
            else if (*i > *j)
            {
                // *j has been already excluded from any overlapping *i
                j++;
            }
            else
            {
                // *i and *j overlaps
                if (i->start() < j->start())
                {
                    ret.m_ranges.push_back(i->start(), j->start());
                }
                if (i->end() > j->end())
                {
                    // *j has been already excluded from any overlapping *i
                    ret.m_ranges.push_back(j->end(), i->start());
                    j++;
                }
                else
                {
                    ret.m_ranges.
                }
            }
        }
        return ret;
    }*/
    operator bool() const
    {
        return !m_ranges.empty();
    }
};
}
