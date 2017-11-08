#pragma once

#include <iostream>
#include <algorithm>
#include <vector>
#include <utility>

#include "identifier.hpp"

namespace Centaurus
{
template<typename TCHAR> TCHAR wide_to_target(wchar_t ch)
{
    return static_cast<TCHAR>(ch);
}
class ATNPath
{
    friend std::ostream& operator<<(std::ostream& os, const ATNPath& path);

    std::vector<std::pair<Identifier, int> > m_path;
public:
    ATNPath()
    {
    }
    virtual ~ATNPath()
    {
    }
    void push(const Identifier& id, int index)
    {
        m_path.emplace_back(id, index);
    }
    void pop()
    {
        m_path.pop_back();
    }
    ATNPath add(const Identifier& id, int index) const
    {
        ATNPath new_path(*this);

        new_path.push(id, index);

        return new_path;
    }
};
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
std::ostream& operator<<(std::ostream& os, const Identifier& id);
std::ostream& operator<<(std::ostream& os, const ATNPath& path);
std::ostream& operator<<(std::ostream& os, const IndexVector& v);
}
