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
std::ostream& operator<<(std::ostream& os, const IndexVector& v);
}
