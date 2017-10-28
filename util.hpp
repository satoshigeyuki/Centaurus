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
class Stream
{
    std::basic_string<TCHAR> m_str;
    std::basic_string<TCHAR>::iterator m_cur;
    int m_line, m_pos;
    bool m_newline_flag;
public:
    using Sentry = std::basic_string<TCHAR>::const_iterator;
    Stream(std::basic_string<TCHAR>&& str)
        : m_str(str), m_line(1), m_pos(0), m_newline_flag(false)
    {
        m_cur = m_str.begin();
    }
    virtual ~Stream
    {
    }
    TCHAR get()
    {
        if (m_cur == m_str.end())
            return 0;
        else
        {
            TCHAR ch = *(m_cur++);
            if (m_newline_flag)
            {
                m_newline_flag = false;
                m_line++;
                m_pos = 0;
            }
            else
            {
                m_pos++;
            }
            if (ch == wide_to_target<TCHAR>(L'\r'))
            {
                if (m_cur == m_str.end())
                    return wide_to_target<TCHAR>(L'\n');
                if (*m_cur == wide_to_target<TCHAR>(L'\n'))
                {
                    m_newline_flag = true;
                    m_pos++;
                    m_cur++;
                }
            }
            else if (ch == wide_to_target<TCHAR>(L'\n'))
            {
                m_newline_flag = true;
            }
            return ch;
        }
    }
    TCHAR skip_whitespace()
    {
        TCHAR ch = get();
        for (; ch != 0; ch = get())
        {
            if (ch != wide_to_target<TCHAR>(L' ') && \
                ch != wide_to_target<TCHAR>(L'\t') && \
                ch != wide_to_target<TCHAR>(L'\r') && \
                ch != wide_to_target<TCHAR>(L'\n'))
            {
                break;
            }
        }
        return ch;
    }
    Sentry sentry()
    {
        return m_cur;
    }
    std::basic_string<TCHAR> cut(const Sentry& sentry)
    {
        return std::basic_string<TCHAR>(sentry, m_cur);
    }
    std::exception unexpected(TCHAR ch)
    {
        std::ostringstream stream;

        stream << "Line " << m_line << ", Pos " << m_pos << ": Unexpected character " << ch;

        std::string msg = stream.str();

        return std::exception(msg.c_str());
    }
};
using WideStream = Stream<wchar_t>;
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
