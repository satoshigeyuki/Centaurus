#pragma once

#include <iostream>
#include <algorithm>
#include <vector>

namespace Centaurus
{
template<typename TCHAR> TCHAR wide_to_target(wchar_t ch)
{
    return static_cast<TCHAR>(ch);
}
template<typename TCHAR>
class SentryStream
{
    std::basic_string<TCHAR> m_str;
    std::basic_string<TCHAR>::iterator m_cur;
public:
    using Sentry = std::basic_string<TCHAR>::const_iterator;
    SentryStream(std::basic_string<TCHAR>&& str)
        : m_str(str)
    {
        m_cur = m_str.begin();
    }
    virtual ~SentryStream
    {
    }
    TCHAR get()
    {
        if (m_cur == m_str.end())
            return 0;
        else
            return *(m_cur++);
    }
    Sentry get_sentry()
    {
        return m_cur;
    }
};
using WideSentryStream = SentryStream<wchar_t>;
class IndexVector : public std::vector<int>
{
public:
    IndexVector()
    {
    }
    explicit IndexVector(size_type n)
        : std::vector<int>(n)
    {
    }
    IndexVector(size_type n, bool reserve_flag = true)
        : std::vector<int>(reserve_flag ? 0 : n)
    {
        if (reserve_flag)
            reserve(n);
    }
    template<typename InputIterator>
    IndexVector(InputIterator a, InputIterator b)
        : std::vector<int>(a, b)
    {
    }
    virtual ~IndexVector()
    {
    }
    IndexVector& operator+=(const IndexVector& v)
    {
        reserve(size() + v.size());
        insert(end(), v.cbegin(), v.cend());
        return *this;
    }
    void sort()
    {
        std::sort(begin(), end());
    }
    /*!
     * @brief Perform destructive sort and copy removing duplicate values
     */
    IndexVector sort_and_unique_copy()
    {
        IndexVector ret;

        std::sort(begin(), end());

        if (!empty())
        {
            ret.push_back(front());

            int prev_value = front();

            for (auto i = begin() + 1; i != end(); i++)
            {
                if (*i != prev_value)
                {
                    ret.push_back(*i);
                    prev_value = *i;
                }
            }
        }

        return ret;
    }
};
std::ostream& operator<<(std::ostream& os, const IndexVector& v)
{
    os << '[';
    for (unsigned int i = 0; i < v.size() - 1; i++)
    {
        os << v[i] << ',';
    }
    if (!v.empty())
        os << v.back();
    os << ']';
    return os;
}
}
